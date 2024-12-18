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
#include "mqtt.h"
#include "i2c.h"
#include "sensors/heartrate.h"

#define BLINK_GPIO 2
#define BLINK_PERIOD 1000

#define BUFFER_SIZE 128

int wifiConnected = 0;
int led_state = 0;

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

void blink(void *pvParameters)
{
    gpio_reset_pin(BLINK_GPIO);
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
    int count = 0;

    while (1)
    {
        count = (count + 1) % 10;
        if (wifiConnected)
            led_state = 0;
        else if (count == 0)
            led_state = !led_state;
        gpio_set_level(BLINK_GPIO, led_state);
        vTaskDelay(BLINK_PERIOD / portTICK_PERIOD_MS / 10);
    }
}

void heartrate(void *pvParameters)
{
    i2c_master_bus_handle_t bus_handle;
    i2c_master_dev_handle_t dev_handle;
    i2c_master_init(&bus_handle, &dev_handle);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    max30102_init(dev_handle, &max30102_configuration);

    int32_t red_data = 0;
    int32_t ir_data = 0;
    int32_t red_data_buffer[BUFFER_SIZE];
    int32_t ir_data_buffer[BUFFER_SIZE];

    uint64_t ir_mean;
    uint64_t red_mean;

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
        remove_dc_part(ir_data_buffer, red_data_buffer, &ir_mean, &red_mean);
        remove_trend_line(ir_data_buffer);
        remove_trend_line(red_data_buffer);
        int heart_rate = calculate_heart_rate(ir_data_buffer);

        printf("Heart rate: %d\n", heart_rate);
        printf("Max30102 Temperature: %.2f\n", temperature);

        double spo2 = spo2_measurement(ir_data_buffer, red_data_buffer, ir_mean, red_mean);
        printf("SPO2 %f\n", spo2);
    }
}

void app_main(void)
{
    nvs_flash_init();

    wifi_init();
    xTaskCreate(blink, "blink", 2048, NULL, 4, NULL);

    ble_init();
    xTaskCreate(randomBattery, "randomBattery", 4096, NULL, 5, NULL);

    xTaskCreate(heartrate, "heartrate", 4096, NULL, 5, NULL);

    while (wifiConnected == 0)
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    mqtt_init();
}