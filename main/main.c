#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "driver/gpio.h"
#include "wifi.h"

#define WIFI_SSID "OnePlus6"
#define WIFI_PASS "12345678"
#define BLINK_GPIO 2
#define BLINK_PERIOD 1000

int connected = 0;
int led_state = 0;

void blink(void *pvParameters)
{
    gpio_reset_pin(BLINK_GPIO);
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
    int count = 0;

    while (1)
    {
        count = (count + 1) % 10;
        if (connected)
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
    wifi_init(WIFI_SSID, WIFI_PASS);
    xTaskCreate(blink, "blink", 2048, NULL, 4, NULL);
}