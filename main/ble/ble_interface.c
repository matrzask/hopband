#include "ble_interface.h"
#include "ble_gap.h"
#include "ble_gatts.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *TAG = "BLE-INTERFACE";

esp_err_t ble_init()
{
    esp_err_t ret;

    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret)
    {
        ESP_LOGE(TAG, "%s initialize controller failed", __func__);
        return ret;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret)
    {
        ESP_LOGE(TAG, "%s enable controller failed", __func__);
        return ret;
    }

    ret = esp_bluedroid_init();
    if (ret)
    {
        ESP_LOGE(TAG, "%s init bluetooth failed", __func__);
        return ret;
    }
    ret = esp_bluedroid_enable();
    if (ret)
    {
        ESP_LOGE(TAG, "%s enable bluetooth failed", __func__);
        return ret;
    }

    ret = ble_gap_init();
    if (ret)
    {
        ESP_LOGE(TAG, "BLE GAP initialization failed");
        return ret;
    }

    ret = ble_gatt_server_init();
    if (ret)
    {
        ESP_LOGE(TAG, "BLE GATT Server initialization failed");
        return ret;
    }

    ESP_LOGI(TAG, "BLE GATT Server ready");
    return ret;
}

esp_err_t ble_deinit()
{
    esp_err_t ret;

    ret = esp_bluedroid_disable();
    if (ret)
    {
        ESP_LOGE(TAG, "disable bluedroid failed");
        return ret;
    }

    ret = esp_bluedroid_deinit();
    if (ret)
    {
        ESP_LOGE(TAG, "deinitialize bluedroid failed");
        return ret;
    }

    ret = esp_bt_controller_disable();
    if (ret)
    {
        ESP_LOGE(TAG, "disable bt controller failed");
        return ret;
    }

    ret = esp_bt_controller_deinit();
    if (ret)
    {
        ESP_LOGE(TAG, "deinitialize bt controller failed");
        return ret;
    }

    return ret;
}