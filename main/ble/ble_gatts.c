#include "ble_gatts.h"

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"

#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"

#include "ble_gap.h"

static const char *TAG = "BLE-GATTS";

#define PROFILE_NUM 1
#define PROFILE_APP_IDX 0
#define ESP_APP_ID 0x55
#define SVC_INST_ID 0

#define PREPARE_BUF_MAX_SIZE 1024

#define ADV_CONFIG_FLAG (1 << 0)
#define SCAN_RSP_CONFIG_FLAG (1 << 1)

static uint16_t gatts_mtu = 23;

typedef struct
{
    uint8_t *prepare_buf;
    int prepare_len;
    uint16_t handle;
} prepare_type_env_t;

static prepare_type_env_t prepare_write_env;
static prepare_type_env_t prepare_read_env;

struct gatts_profile_inst
{
    esp_gatts_cb_t gatts_cb;
    uint16_t gatts_if;
    uint16_t app_id;
    uint16_t conn_id;
    uint16_t service_handle;
    esp_gatt_srvc_id_t service_id;
    uint16_t char_handle;
    esp_bt_uuid_t char_uuid;
    esp_gatt_perm_t perm;
    esp_gatt_char_prop_t property;
    uint16_t descr_handle;
    esp_bt_uuid_t descr_uuid;
};

typedef struct
{
    uint16_t gatts_if;
    uint16_t conn_id;
    bool has_conn;
} gatts_profile_inst_t;

static gatts_profile_inst_t profile;

static void gatts_profile_event_handler(esp_gatts_cb_event_t event,
                                        esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);

/* One gatt-based profile one app_id and one gatts_if, this array will store the gatts_if returned by ESP_GATTS_REG_EVT */
static struct gatts_profile_inst bt_profile_tab[PROFILE_NUM] = {
    [PROFILE_APP_IDX] = {
        .gatts_cb = gatts_profile_event_handler,
        .gatts_if = ESP_GATT_IF_NONE, /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
    },
};

static void gatts_profile_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    switch (event)
    {
    case ESP_GATTS_REG_EVT:
    {
        profile.gatts_if = gatts_if;
        ble_gap_setAdvertisingData();

        /*
         *  Add calls to register new attribute tables here
         *  Template:
         *  esp_err_t create_attr_ret = esp_ble_gatts_create_attr_tab(#SERVICE GATT DB#, gatts_if, #SERVICE MAX INDEX#, SVC_INST_ID);
         *  if (create_attr_ret){
         *      ESP_LOGE(TAG, "create attr table failed, error code = %x", create_attr_ret);
         *  }
         */

        /* End calls to register external service attribute tables */
    }
    break;
    default:
        break;
    }
}

static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    /* If event is register event, store the gatts_if for each profile */
    if (event == ESP_GATTS_REG_EVT)
    {
        if (param->reg.status == ESP_GATT_OK)
        {
            bt_profile_tab[PROFILE_APP_IDX].gatts_if = gatts_if;
        }
        else
        {
            ESP_LOGE(TAG, "reg app failed, app_id %04x, status %d",
                     param->reg.app_id,
                     param->reg.status);
            return;
        }
    }
    do
    {
        int idx;
        for (idx = 0; idx < PROFILE_NUM; idx++)
        {
            /* ESP_GATT_IF_NONE, not specify a certain gatt_if, need to call every profile cb function */
            if (gatts_if == ESP_GATT_IF_NONE || gatts_if == bt_profile_tab[idx].gatts_if)
            {
                if (bt_profile_tab[idx].gatts_cb)
                {
                    bt_profile_tab[idx].gatts_cb(event, gatts_if, param);
                }
            }
        }
    } while (0);
}

// Helper function for services which fills in the gatts_if and conn_id
esp_err_t ble_gatt_server_notify_indicate(uint16_t attr_handle, uint16_t value_len, uint8_t *value, bool need_confirm)
{
    if (profile.has_conn)
    {
        // the size of notify_data[] need less than MTU size
        return esp_ble_gatts_send_indicate(profile.gatts_if, profile.conn_id, attr_handle, value_len, value, need_confirm);
    }
    return ESP_FAIL;
}

esp_err_t ble_gatt_server_init(void)
{
    esp_err_t err;

    err = esp_ble_gatts_register_callback(gatts_event_handler);
    if (err)
    {
        ESP_LOGE(TAG, "Gatts register failed with err: %x", err);
        return err;
    }

    err = esp_ble_gatts_app_register(ESP_APP_ID);
    if (err)
    {
        ESP_LOGE(TAG, "App register failed with err: %x", err);
        return err;
    }

    err = esp_ble_gatt_set_local_mtu(517);
    if (err)
    {
        ESP_LOGE(TAG, "set local  MTU failed, error code = %x", err);
    }

    return err;
}