#include "wifi_service.h"
#include "../ble_common.h"
#include "../ble_gatts.h"
#include <string.h>
#include "esp_gatts_api.h"
#include "esp_log.h"

#define TAG "BLE-WIFI-SERVICE"
#define STR_LEN 32

uint16_t wifi_handle_table[WIFI_SERV_NUM_ATTR];

const uint16_t WIFI_SERV_uuid = 0xFF00;
const uint16_t WIFI_SERV_CHAR_ssid_uuid = 0xFF01;
const uint16_t WIFI_SERV_CHAR_pass_uuid = 0xFF02;

static const uint8_t WIFI_SERV_CHAR_ssid_descr[] = "Wifi SSID";
static const uint8_t WIFI_SERV_CHAR_pass_descr[] = "Wifi Password";

static uint8_t WIFI_SERV_CHAR_ssid_ccc[2] = {0x00, 0x00};
static uint8_t WIFI_SERV_CHAR_pass_ccc[2] = {0x00, 0x00};

static uint8_t WIFI_SERV_CHAR_ssid_val[STR_LEN] = "";
static uint8_t WIFI_SERV_CHAR_pass_val[STR_LEN] = "";

static uint16_t ssid_val_len = 0;
static uint16_t pass_val_len = 0;

const esp_gatts_attr_db_t wifi_serv_gatt_db[WIFI_SERV_NUM_ATTR] =
    {
        [WIFI_SERV] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&primary_service_uuid, ESP_GATT_PERM_READ, sizeof(uint16_t), sizeof(WIFI_SERV_uuid), (uint8_t *)&WIFI_SERV_uuid}},

        [WIFI_SSID_CHAR] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ, CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_write_notify}},
        [WIFI_SSID_VAL] = {{ESP_GATT_RSP_BY_APP}, {ESP_UUID_LEN_16, (uint8_t *)&WIFI_SERV_CHAR_ssid_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, sizeof(uint16_t), 0, NULL}},
        [WIFI_SSID_DESCR] = {{ESP_GATT_RSP_BY_APP}, {ESP_UUID_LEN_16, (uint8_t *)&character_description, ESP_GATT_PERM_READ, CHAR_DECLARATION_SIZE, 0, NULL}},
        [WIFI_SSID_CFG] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_client_config_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, sizeof(uint16_t), sizeof(WIFI_SERV_CHAR_ssid_ccc), (uint8_t *)WIFI_SERV_CHAR_ssid_ccc}},

        [WIFI_PASS_CHAR] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ, CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_write_notify}},
        [WIFI_PASS_VAL] = {{ESP_GATT_RSP_BY_APP}, {ESP_UUID_LEN_16, (uint8_t *)&WIFI_SERV_CHAR_pass_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, sizeof(uint16_t), 0, NULL}},
        [WIFI_PASS_DESCR] = {{ESP_GATT_RSP_BY_APP}, {ESP_UUID_LEN_16, (uint8_t *)&character_description, ESP_GATT_PERM_READ, CHAR_DECLARATION_SIZE, 0, NULL}},
        [WIFI_PASS_CFG] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_client_config_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, sizeof(uint16_t), sizeof(WIFI_SERV_CHAR_pass_ccc), (uint8_t *)WIFI_SERV_CHAR_pass_ccc}},
};

uint16_t getAttributeIndexByWifiHandle(uint16_t attributeHandle)
{
    // Get the attribute index in the attribute table by the returned handle

    uint16_t attrIndex = WIFI_SERV_NUM_ATTR;
    uint16_t index;

    for (index = 0; index < WIFI_SERV_NUM_ATTR; ++index)
    {
        if (wifi_handle_table[index] == attributeHandle)
        {
            attrIndex = index;
            break;
        }
    }

    return attrIndex;
}

void handleWifiReadEvent(int attrIndex, esp_ble_gatts_cb_param_t *param, esp_gatt_rsp_t *rsp)
{
    switch (attrIndex)
    {
    // Characteristic read values
    case WIFI_SSID_VAL:
        memset(rsp->attr_value.value, 0, sizeof(rsp->attr_value.value));
        memcpy(rsp->attr_value.value, WIFI_SERV_CHAR_ssid_val, sizeof(WIFI_SERV_CHAR_ssid_val));
        rsp->attr_value.len = ssid_val_len;
        break;

    case WIFI_PASS_VAL:
        memset(rsp->attr_value.value, 0, sizeof(rsp->attr_value.value));
        memcpy(rsp->attr_value.value, WIFI_SERV_CHAR_pass_val, sizeof(WIFI_SERV_CHAR_pass_val));
        rsp->attr_value.len = pass_val_len;
        break;

    // Characteristic descriptions
    case WIFI_SSID_DESCR:
        memset(rsp->attr_value.value, 0, sizeof(rsp->attr_value.value));
        memcpy(rsp->attr_value.value, WIFI_SERV_CHAR_ssid_descr, sizeof(WIFI_SERV_CHAR_ssid_descr));
        rsp->attr_value.len = sizeof(WIFI_SERV_CHAR_ssid_descr);
        break;

    case WIFI_PASS_DESCR:
        memset(rsp->attr_value.value, 0, sizeof(rsp->attr_value.value));
        memcpy(rsp->attr_value.value, WIFI_SERV_CHAR_pass_descr, sizeof(WIFI_SERV_CHAR_pass_descr));
        rsp->attr_value.len = sizeof(WIFI_SERV_CHAR_pass_descr);
        break;
    }
}

void handleWifiWriteEvent(int attrIndex, const uint8_t *char_val_p, uint16_t char_val_len)
{
    switch (attrIndex)
    {
    case WIFI_SSID_VAL:
        // This prints the first byte written to the characteristic
        ESP_LOGI(TAG, "Wifi SSID characteristic written with %.*s", char_val_len, char_val_p);
        memset(WIFI_SERV_CHAR_ssid_val, 0, sizeof(WIFI_SERV_CHAR_ssid_val));
        memcpy(WIFI_SERV_CHAR_ssid_val, char_val_p, char_val_len > STR_LEN ? STR_LEN : char_val_len);
        ssid_val_len = char_val_len > STR_LEN ? STR_LEN : char_val_len;
        break;

    case WIFI_PASS_VAL:
        ESP_LOGI(TAG, "Wifi PASS characteristic written with %.*s", char_val_len, char_val_p);
        memset(WIFI_SERV_CHAR_pass_val, 0, sizeof(WIFI_SERV_CHAR_pass_val));
        memcpy(WIFI_SERV_CHAR_pass_val, char_val_p, char_val_len > STR_LEN ? STR_LEN : char_val_len);
        pass_val_len = char_val_len > STR_LEN ? STR_LEN : char_val_len;
        break;
    default:
        break;
    }
}