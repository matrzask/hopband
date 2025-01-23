#include "heartrate_service.h"
#include "../ble_common.h"
#include "../ble_gatts.h"
#include <string.h>
#include "esp_gatts_api.h"
#include "esp_log.h"

#define TAG "BLE-HEARTRATE-SERVICE"

uint16_t heartrate_handle_table[HEARTRATE_SERV_NUM_ATTR];

const uint16_t HEARTRATE_SERV_uuid = 0x180D;
static const uint16_t HEARTRATE_CHAR_uuid = 0xFF03;
static const uint16_t SPO2_CHAR_uuid = 0xFF04;

static uint8_t HEARTRATE_VAL_val[2] = {0x00, 0x00};
static uint8_t SPO2_VAL_val[2] = {0x00, 0x00};

const esp_gatts_attr_db_t heartrate_serv_gatt_db[HEARTRATE_SERV_NUM_ATTR] =
    {
        [HEARTRATE_SERV] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&primary_service_uuid, ESP_GATT_PERM_READ, sizeof(uint16_t), sizeof(HEARTRATE_SERV_uuid), (uint8_t *)&HEARTRATE_SERV_uuid}},

        [HEARTRATE_CHAR] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ, CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_notify}},
        [HEARTRATE_VAL] = {{ESP_GATT_RSP_BY_APP}, {ESP_UUID_LEN_16, (uint8_t *)&HEARTRATE_CHAR_uuid, ESP_GATT_PERM_READ, sizeof(HEARTRATE_VAL_val), sizeof(HEARTRATE_VAL_val), (uint8_t *)HEARTRATE_VAL_val}},
        [HEARTRATE_CFG] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_client_config_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, sizeof(uint16_t), sizeof(uint16_t), (uint8_t *)HEARTRATE_VAL_val}},

        [SPO2_CHAR] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ, CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_notify}},
        [SPO2_VAL] = {{ESP_GATT_RSP_BY_APP}, {ESP_UUID_LEN_16, (uint8_t *)&SPO2_CHAR_uuid, ESP_GATT_PERM_READ, sizeof(SPO2_VAL_val), sizeof(SPO2_VAL_val), (uint8_t *)SPO2_VAL_val}},
        [SPO2_CFG] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_client_config_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, sizeof(uint16_t), sizeof(uint16_t), (uint8_t *)SPO2_VAL_val}},
};

uint16_t getAttributeIndexByHeartrateHandle(uint16_t attributeHandle)
{
    uint16_t attrIndex = HEARTRATE_SERV_NUM_ATTR;
    for (uint16_t index = 0; index < HEARTRATE_SERV_NUM_ATTR; ++index)
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
    case HEARTRATE_VAL:
        memset(rsp->attr_value.value, 0, sizeof(rsp->attr_value.value));
        memcpy(rsp->attr_value.value, HEARTRATE_VAL_val, sizeof(HEARTRATE_VAL_val));
        rsp->attr_value.len = sizeof(HEARTRATE_VAL_val);
        break;

    case SPO2_VAL:
        memset(rsp->attr_value.value, 0, sizeof(rsp->attr_value.value));
        memcpy(rsp->attr_value.value, SPO2_VAL_val, sizeof(SPO2_VAL_val));
        rsp->attr_value.len = sizeof(SPO2_VAL_val);
        break;
    }
}

void updateHeartrateValues(uint16_t heartrate, uint16_t spo2)
{
    memcpy(HEARTRATE_VAL_val, &heartrate, sizeof(heartrate));
    memcpy(SPO2_VAL_val, &spo2, sizeof(spo2));

    ble_gatt_server_notify_indicate(
        heartrate_handle_table[HEARTRATE_VAL],
        sizeof(HEARTRATE_VAL_val),
        HEARTRATE_VAL_val,
        false);

    ble_gatt_server_notify_indicate(
        heartrate_handle_table[SPO2_VAL],
        sizeof(SPO2_VAL_val),
        SPO2_VAL_val,
        false);
}