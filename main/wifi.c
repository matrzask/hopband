#include "wifi.h"

#include "esp_wifi.h"
#include <esp_http_client.h>
#include "esp_log.h"

#define TEST_WEBSITE "http://example.com"

#define TAG "HopBand-Wifi"

extern int wifiConnected;

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
        ESP_LOGI(TAG, "WIFI CONNECTED");
        break;
    case WIFI_EVENT_STA_DISCONNECTED:
        wifiConnected = 0;
        ESP_LOGI(TAG, "WiFi lost connection");
        esp_wifi_connect();
        ESP_LOGI(TAG, "Trying to reconnect...");
        break;
    case IP_EVENT_STA_GOT_IP:
        wifiConnected = 1;
        ESP_LOGI(TAG, "Wifi got IP...");
        xTaskCreate(getHtml, "getHtml", 4096, NULL, 5, NULL);
        break;
    default:
        break;
    }
}

void wifi_init(char *ssid, char *pass)
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
            .ssid = "",
            .password = "",
        }};
    strcpy((char *)wifi_configuration.sta.ssid, ssid);
    strcpy((char *)wifi_configuration.sta.password, pass);
    esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_configuration);
    esp_wifi_start();
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_connect();
}