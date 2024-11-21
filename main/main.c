#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "driver/gpio.h"
#include <esp_http_client.h>

#define WIFI_SSID "OnePlus6"
#define WIFI_PASS "12345678"
#define TEST_WEBSITE "http://example.com"
#define BLINK_GPIO 2
#define BLINK_PERIOD 1000

int connected = 0;
int led_state = 0;

static const char *TAG = "HopBand";

esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id)
    {
    case HTTP_EVENT_ERROR:
        ESP_LOGI(TAG, "HTTP GET ERROR");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGI(TAG, "HTTP GET CONNECTED");
        break;
    case HTTP_EVENT_ON_DATA:
        if (evt->data_len > 0)
        {
            printf("%.*s", evt->data_len, (char *)evt->data);
        }
        break;
    default:
        break;
    }
    return ESP_OK;
}

void getHtml(void *pvParameters)
{
    esp_http_client_config_t config = {
        .url = TEST_WEBSITE,
        .event_handler = _http_event_handler,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_http_client_perform(client);

    esp_http_client_cleanup(client);

    vTaskDelete(NULL);
}

static void wifi_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    switch (event_id)
    {
    case WIFI_EVENT_STA_START:
        ESP_LOGI(TAG, "WIFI CONNECTING....");
        break;
    case WIFI_EVENT_STA_CONNECTED:
        connected = 1;
        ESP_LOGI(TAG, "WIFI CONNECTED");
        break;
    case WIFI_EVENT_STA_DISCONNECTED:
        connected = 0;
        ESP_LOGI(TAG, "WiFi lost connection");
        esp_wifi_connect();
        ESP_LOGI(TAG, "Trying to reconnect...");
        break;
    case IP_EVENT_STA_GOT_IP:
        ESP_LOGI(TAG, "Wifi got IP...");
        xTaskCreate(getHtml, "getHtml", 4096, NULL, 5, NULL);
        break;
    default:
        break;
    }
}

void wifi_connection()
{
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t wifi_initiation = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&wifi_initiation);
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL);
    wifi_config_t wifi_configuration = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        }};
    esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_configuration);
    esp_wifi_start();
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_connect();
}

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
    wifi_connection();
    xTaskCreate(blink, "blink", 2048, NULL, 4, NULL);
}