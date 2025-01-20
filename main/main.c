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
#include "ble/services/battery_service.h"
#include "ble/services/wifi_service.h"
#include "mqtt.h"
#include "i2c.h"
#include "sensors/heartrate.h"
#include "sensors/accmeter.h"
#include "esp_log.h"
#include "thermistor.h"
#include "led.h"

#define BUFFER_SIZE 128
#define TAG "main"

#define BUTTON_GPIO 15

extern int wifiConnected;
extern int configMode;
int led_state = 0;
int wifiServiceFlag = 0;

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

void randomBattery(void *pvParameters)
{
    while (1)
    {
        int batteryValue = rand() % 100;
        updateBatteryValue((u_int8_t)batteryValue);
        char message[50];
        sprintf(message, "Battery level: %d%%", batteryValue);
        publish_message("/battery/level", message);
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}

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
    int32_t red_data_buffer[BUFFER_SIZE];
    int32_t ir_data_buffer[BUFFER_SIZE];

    while (1)
    {
        for (int i = 0; i < BUFFER_SIZE; i++)
        {
            read_max30102_fifo(dev_handle, &red_data, &ir_data);
            ir_data_buffer[i] = ir_data;
            red_data_buffer[i] = red_data;
            ir_data = 0;
            red_data = 0;
            vTaskDelay(pdMS_TO_TICKS(40));
        }

        float temperature = get_max30102_temp(dev_handle);
        int heart_rate = calculate_heart_rate(ir_data_buffer);
        double spo2 = spo2_measurement(ir_data_buffer, red_data_buffer);

        printf("Heart rate: %d\n", heart_rate);
        printf("Max30102 Temperature: %.2f\n", temperature);
        printf("SPO2 %f\n\n", spo2);
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
        int16_t x = adxl345_read_x(dev_handle);
        int16_t y = adxl345_read_y(dev_handle);
        int16_t z = adxl345_read_z(dev_handle);

        printf("X: %d, Y: %d, Z: %d\n", x, y, z);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
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
    xTaskCreate(randomBattery, "randomBattery", 4096, NULL, 5, NULL);

    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_NEGEDGE, // Trigger on falling edge (button press)
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << BUTTON_GPIO),
        .pull_up_en = GPIO_PULLUP_ENABLE, // Enable internal pull-up resistor
    };
    gpio_config(&io_conf);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_GPIO, button_isr_handler, NULL);

    // xTaskCreate(heartrate, "heartrate", 4096, (void *)bus_handle, 5, NULL);
    // xTaskCreate(accelerometer, "accelerometer", 4096, (void *)bus_handle, 5, NULL);

    while (wifiConnected == 0)
    {
        if (wifiServiceFlag)
        {
            showWifiService();
            wifiServiceFlag = false;
        }
        printf("Temperature: %.2f\n", getTempReading());
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    mqtt_init();

    while (1)
    {
        if (wifiServiceFlag)
        {
            showWifiService();
            wifiServiceFlag = false;
        }
        printf("Temperature: %.2f\n", getTempReading());
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}