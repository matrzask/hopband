#include "mqtt.h"

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "esp_event.h"

#include "freertos/FreeRTOS.h"

#include "esp_log.h"
#include "mqtt_client.h"

#include "wifi.h"

#define TAG "MQTT"
#define CONFIG_BROKER_URL "mqtt://192.168.132.100:1883"

esp_mqtt_client_handle_t client;
char *id;
extern int steps;
extern int heart_rate;
extern int spo2;

int mqttConnected = 0;

static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0)
    {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32 "", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    switch ((esp_mqtt_event_id_t)event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        mqttConnected = 1;
        char topic[64];
        snprintf(topic, sizeof(topic), "/%s/steps", id);
        esp_mqtt_client_subscribe(client, topic, 1);
        snprintf(topic, sizeof(topic), "/%s/heartrate", id);
        esp_mqtt_client_subscribe(client, topic, 1);
        snprintf(topic, sizeof(topic), "/%s/spo2", id);
        esp_mqtt_client_subscribe(client, topic, 1);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        mqttConnected = 0;
        break;
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        // ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        printf("Received topic: %.*s\n", event->topic_len, event->topic);
        if (event->data_len > 0)
        {
            char buf[32];
            int len = event->data_len < (sizeof(buf) - 1) ? event->data_len : (sizeof(buf) - 1);
            memcpy(buf, event->data, len);
            buf[len] = '\0';

            char topic_buf[64];
            snprintf(topic_buf, sizeof(topic_buf), "/%s/steps", id);
            if (strncmp(event->topic, topic_buf, event->topic_len) == 0 && strlen(topic_buf) == event->topic_len)
            {
                char *endptr;
                int received_steps = strtol(buf, &endptr, 10);
                if (*endptr == '\0')
                {
                    steps = received_steps;
                    ESP_LOGI(TAG, "Steps updated: %d", steps);
                }
                else
                {
                    ESP_LOGE(TAG, "Invalid steps data received: %.*s", event->data_len, event->data);
                }
            }
            else
            {
                snprintf(topic_buf, sizeof(topic_buf), "/%s/heartrate", id);
                if (strncmp(event->topic, topic_buf, event->topic_len) == 0 && strlen(topic_buf) == event->topic_len)
                {
                    char *endptr;
                    int received_hr = strtol(buf, &endptr, 10);
                    if (*endptr == '\0')
                    {
                        heart_rate = received_hr;
                        ESP_LOGI(TAG, "Heart rate updated: %d", heart_rate);
                    }
                    else
                    {
                        ESP_LOGE(TAG, "Invalid heartrate data received: %.*s", event->data_len, event->data);
                    }
                }
                else
                {
                    snprintf(topic_buf, sizeof(topic_buf), "/%s/spo2", id);
                    if (strncmp(event->topic, topic_buf, event->topic_len) == 0 && strlen(topic_buf) == event->topic_len)
                    {
                        char *endptr;
                        int received_spo2 = strtol(buf, &endptr, 10);
                        if (*endptr == '\0')
                        {
                            spo2 = received_spo2;
                            ESP_LOGI(TAG, "SpO2 updated: %d", spo2);
                        }
                        else
                        {
                            ESP_LOGE(TAG, "Invalid SpO2 data received: %.*s", event->data_len, event->data);
                        }
                    }
                    else
                    {
                        ESP_LOGE(TAG, "Unknown topic received: %.*s", event->topic_len, event->topic);
                    }
                }
            }
        }
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT)
        {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno", event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
        }
        break;
    default:
        break;
    }
}

void publish_message(const char *topic, const char *data, int len) // set len to 0 to use data length
{
    if (mqttConnected == 0)
    {
        return;
    }
    char full_topic[256];
    snprintf(full_topic, sizeof(full_topic), "/%s%s", id, topic);
    esp_mqtt_client_publish(client, full_topic, data, len, 1, 0);
}

void mqtt_init(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = CONFIG_BROKER_URL,
    };

    uint8_t *mac = get_mac_address();
    id = (char *)malloc(13);
    sprintf(id, "esp32-%02x%02x%02x", mac[3], mac[4], mac[5]);
    ESP_LOGI(TAG, "Device ID: %s", id);

    client = esp_mqtt_client_init(&mqtt_cfg);
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
    ESP_LOGI(TAG, "MQTT client started");
}