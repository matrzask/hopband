#include "heartrate_service.h"
#include "../ble_common.h"
#include <string.h>
#include "esp_gatts_api.h"
#include "esp_log.h"

#define TAG "BLE-HEARTRATE-SERVICE"

uint16_t heartrate_handle_table[HEARTRATE_SERV_NUM_ATTR];

const uint16_t HEARTRATE_SERV_uuid = 0x180D;
static const uint16_t HEARTRATE_SERV_CHAR_info_uuid = 0x2A37;

// static const uint8_t HEARTRATE_SERV_descr[] = "Heartrate Service";
static const uint8_t HEARTRATE_SERV_CHAR_info_descr[] = "Heartrate Value";

static uint8_t HEARTRATE_SERV_CHAR_info_ccc[2] = {0x00, 0x00};

static uint8_t HEARTRATE_SERV_CHAR_info_val[20] = {0x00};

const esp_gatts_attr_db_t heartrate_serv_gatt_db[HEARTRATE_SERV_NUM_ATTR] =
    {
        [HEARTRATE_SERV] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&primary_service_uuid, ESP_GATT_PERM_READ, sizeof(uint16_t), sizeof(HEARTRATE_SERV_uuid), (uint8_t *)&HEARTRATE_SERV_uuid}},

        [HEARTRATE_INFO_CHAR] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ, CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_notify}},
        [HEARTRATE_INFO_VAL] = {{ESP_GATT_RSP_BY_APP}, {ESP_UUID_LEN_16, (uint8_t *)&HEARTRATE_SERV_CHAR_info_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, sizeof(uint16_t), 0, NULL}},
        [HEARTRATE_INFO_DESCR] = {{ESP_GATT_RSP_BY_APP}, {ESP_UUID_LEN_16, (uint8_t *)&character_description, ESP_GATT_PERM_READ, CHAR_DECLARATION_SIZE, 0, NULL}},
        [HEARTRATE_INFO_CFG] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_client_config_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, sizeof(uint16_t), sizeof(HEARTRATE_SERV_CHAR_info_ccc), (uint8_t *)HEARTRATE_SERV_CHAR_info_ccc}},
};

uint16_t getAttributeIndexByHeartrateHandle(uint16_t attributeHandle)
{
    // Get the attribute index in the attribute table by the returned handle

    uint16_t attrIndex = HEARTRATE_SERV_NUM_ATTR;
    uint16_t index;

    for (index = 0; index < HEARTRATE_SERV_NUM_ATTR; ++index)
    {
        if (heartrate_handle_table[index] == attributeHandle)
        {
            attrIndex = index;
            break;
        }
    }

    return attrIndex;
}

void handleHeartrateReadEvent(int attrIndex, esp_ble_gatts_cb_param_t *param, esp_gatt_rsp_t *rsp)
{
    switch (attrIndex)
    {
    // Characteristic read values
    case HEARTRATE_INFO_VAL:
        memset(rsp->attr_value.value, 0, sizeof(rsp->attr_value.value));
        memcpy(rsp->attr_value.value, HEARTRATE_SERV_CHAR_info_val, sizeof(HEARTRATE_SERV_CHAR_info_val));
        rsp->attr_value.len = sizeof(HEARTRATE_SERV_CHAR_info_val);
        break;

    // Characteristic descriptions
    case HEARTRATE_INFO_DESCR:
        memset(rsp->attr_value.value, 0, sizeof(rsp->attr_value.value));
        memcpy(rsp->attr_value.value, HEARTRATE_SERV_CHAR_info_descr, sizeof(HEARTRATE_SERV_CHAR_info_descr));
        rsp->attr_value.len = sizeof(HEARTRATE_SERV_CHAR_info_descr);
        break;
    }
}