#include "nvstorage.h"
#include "nvs.h"
#include "esp_err.h"
#include "esp_log.h"

#define TAG "NVSTORAGE"

char *get_nvs_value(char *key)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
        return NULL;
    }

    size_t required_size;
    err = nvs_get_str(handle, key, NULL, &required_size);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error (%s) reading!", esp_err_to_name(err));
        return NULL;
    }

    char *value = malloc(required_size);
    err = nvs_get_str(handle, key, value, &required_size);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error (%s) reading!", esp_err_to_name(err));
        return NULL;
    }

    nvs_close(handle);
    return value;
}

void set_nvs_value(char *key, char *value)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
        return;
    }

    err = nvs_set_str(handle, key, value);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error (%s) writing!", esp_err_to_name(err));
        return;
    }

    err = nvs_commit(handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error (%s) committing!", esp_err_to_name(err));
        return;
    }

    nvs_close(handle);
}