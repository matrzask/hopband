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

#define BLINK_GPIO 2
#define BLINK_PERIOD 1000

int wifiConnected = 0;
int led_state = 0;

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

void app_main(void)
{
    nvs_flash_init();

    wifi_init();
    xTaskCreate(blink, "blink", 2048, NULL, 4, NULL);

    ble_init();
    xTaskCreate(randomBattery, "randomBattery", 4096, NULL, 5, NULL);

    while (wifiConnected == 0)
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    mqtt_init();
}