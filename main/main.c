#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "esp_system.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "driver/gpio.h"
#include "wifi.h"
#include "ble/ble_interface.h"
#include "ble/services/wifi_service.h"
#include "mqtt.h"
#include "i2c.h"
#include "sensors/accmeter.h"
#include "esp_log.h"
#include "led.h"
#include "ST7789.h"
#include "LVGL_Driver.h"
#include "lvgl__lvgl/src/font/lv_font.h"
#include "sensors/heartrate.h"

#define BUFFER_SIZE 128
#define TAG "main"

#define BUTTON_GPIO 15

extern int wifiConnected;
extern int configMode;
extern int delay;
int led_state = 0;
int wifiServiceFlag = 0;
int steps = 0;
int heart_rate = 0;
int spo2 = 0;

max_config max30102_configuration = {

    .INT_EN_1.A_FULL_EN = 1,
    .INT_EN_1.PPG_RDY_EN = 1,
    .INT_EN_1.ALC_OVF_EN = 0,
    .INT_EN_1.PROX_INT_EN = 0,

    .INT_EN_2.DIE_TEMP_RDY_EN = 0,

    .FIFO_WRITE_PTR.FIFO_WR_PTR = 0,

    .OVEF_COUNTER.OVF_COUNTER = 0,

    .FIFO_READ_PTR.FIFO_RD_PTR = 0,

    .FIFO_CONF.SMP_AVE = 0b010,
    .FIFO_CONF.FIFO_ROLLOVER_EN = 1,
    .FIFO_CONF.FIFO_A_FULL = 0,

    .MODE_CONF.SHDN = 0,
    .MODE_CONF.RESET = 0,
    .MODE_CONF.MODE = 0b011, // SPO2 mode

    .SPO2_CONF.SPO2_ADC_RGE = 0b01, // 16384 nA
    .SPO2_CONF.SPO2_SR = 0b001,     // 200 samples per second
    .SPO2_CONF.LED_PW = 0b10,

    .LED1_PULSE_AMP.LED1_PA = 0x24,
    .LED2_PULSE_AMP.LED2_PA = 0x24,

    .PROX_LED_PULS_AMP.PILOT_PA = 0X7F,

    .MULTI_LED_CONTROL1.SLOT2 = 0,
    .MULTI_LED_CONTROL1.SLOT1 = 0,

    .MULTI_LED_CONTROL2.SLOT4 = 0,
    .MULTI_LED_CONTROL2.SLOT3 = 0,
};

void button_isr_handler(void *arg)
{
    configMode = 1;
    wifiServiceFlag = 1;
}

void heartrate(void *pvParameters)
{
    i2c_master_dev_handle_t dev_handle;
    i2c_master_bus_handle_t bus_handle = (i2c_master_bus_handle_t)pvParameters;
    i2c_add_device(&bus_handle, &dev_handle, MAX30102_I2C_ADDR);

    vTaskDelay(100 / portTICK_PERIOD_MS);
    max30102_init(dev_handle, &max30102_configuration);

    int32_t red_data = 0;
    int32_t ir_data = 0;

    while (1)
    {
        read_max30102_fifo(dev_handle, &red_data, &ir_data);

        publish_message("/max30102/ir", (const char *)&ir_data, sizeof(ir_data));
        publish_message("/max30102/red", (const char *)&red_data, sizeof(red_data));

        ir_data = 0;
        red_data = 0;
        vTaskDelay(pdMS_TO_TICKS(40));
    }
}

void accelerometer(void *pvParameters)
{
    i2c_master_dev_handle_t dev_handle;
    i2c_master_bus_handle_t bus_handle = (i2c_master_bus_handle_t)pvParameters;
    i2c_add_device(&bus_handle, &dev_handle, ADXL345_I2C_ADDRESS);

    vTaskDelay(100 / portTICK_PERIOD_MS);
    adxl345_init(dev_handle);

    while (1)
    {
        float x = adxl345_read_x(dev_handle);
        float y = adxl345_read_y(dev_handle);
        float z = adxl345_read_z(dev_handle);

        uint8_t x_bytes[4];
        uint8_t y_bytes[4];
        uint8_t z_bytes[4];

        memcpy(x_bytes, &x, sizeof(x));
        memcpy(y_bytes, &y, sizeof(y));
        memcpy(z_bytes, &z, sizeof(z));

        publish_message("/adxl345/x", (const char *)x_bytes, 4);
        publish_message("/adxl345/y", (const char *)y_bytes, 4);
        publish_message("/adxl345/z", (const char *)z_bytes, 4);

        // ESP_LOGI("accel", "X: %.5f, Y: %.5f, Z: %.5f", x, y, z);

        vTaskDelay(67 / portTICK_PERIOD_MS); // 15Hz
    }
}

void app_main(void)
{
    nvs_flash_init();

    i2c_master_bus_handle_t bus_handle;
    i2c_master_init(&bus_handle);

    wifi_init();
    xTaskCreate(ledTask, "blink", 4096, NULL, 4, NULL);

    ble_init();

    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_NEGEDGE, // Trigger on falling edge (button press)
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << BUTTON_GPIO),
        .pull_up_en = GPIO_PULLUP_ENABLE, // Enable internal pull-up resistor
    };
    gpio_config(&io_conf);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_GPIO, button_isr_handler, NULL);

    xTaskCreate(accelerometer, "accelerometer", 4096, (void *)bus_handle, 5, NULL);
    xTaskCreate(heartrate, "heartrate", 4096, (void *)bus_handle, 5, NULL);
    LCD_Init();
    LVGL_Init();

    lv_obj_t *steps_label = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_font(steps_label, &lv_font_montserrat_32, 0);
    lv_label_set_text_fmt(steps_label, "Steps: %d", steps);
    lv_obj_align(steps_label, LV_ALIGN_CENTER, 0, -25);

    lv_obj_t *hr_spo2_label = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_font(hr_spo2_label, &lv_font_montserrat_32, 0);
    lv_label_set_text_fmt(hr_spo2_label, "HR: %d  SpO2: %d%%", heart_rate, spo2);
    lv_obj_align(hr_spo2_label, LV_ALIGN_CENTER, 0, 25);

    while (wifiConnected == 0)
    {
        if (wifiServiceFlag)
        {
            showWifiService();
            wifiServiceFlag = false;
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    mqtt_init();

    // lv_label_set_text_fmt(label, "Steps: %d", 12345);

    while (1)
    {
        if (wifiServiceFlag)
        {
            showWifiService();
            wifiServiceFlag = false;
        }
        lv_label_set_text_fmt(steps_label, "Steps: %d", steps);
        lv_label_set_text_fmt(hr_spo2_label, "HR: %d  SpO2: %d%%", heart_rate, spo2);
        lv_timer_handler();
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}