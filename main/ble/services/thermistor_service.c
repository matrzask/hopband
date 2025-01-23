#include "thermistor_service.h"
#include "../ble_common.h"
#include "../ble_gatts.h"
#include <string.h>
#include "esp_gatts_api.h"
#include "esp_log.h"

#define TAG "BLE-THERM-SERVICE"

uint16_t therm_handle_table[THERM_SERV_NUM_ATTR];

const uint16_t THERM_SERV_uuid = 0x181A;
static const uint16_t THERM_TEMP_CHAR_uuid = 0x2A6E;

static uint8_t THERM_TEMP_VAL_val[4] = {0x00, 0x00, 0x00, 0x00};

const esp_gatts_attr_db_t therm_serv_gatt_db[THERM_SERV_NUM_ATTR] =
    {
        [THERM_SERV] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&primary_service_uuid, ESP_GATT_PERM_READ, sizeof(uint16_t), sizeof(THERM_SERV_uuid), (uint8_t *)&THERM_SERV_uuid}},

        [THERM_TEMP_CHAR] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ, CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_notify}},
        [THERM_TEMP_VAL] = {{ESP_GATT_RSP_BY_APP}, {ESP_UUID_LEN_16, (uint8_t *)&THERM_TEMP_CHAR_uuid, ESP_GATT_PERM_READ, sizeof(THERM_TEMP_VAL_val), sizeof(THERM_TEMP_VAL_val), (uint8_t *)THERM_TEMP_VAL_val}},
        [THERM_TEMP_CFG] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_client_config_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, sizeof(uint16_t), sizeof(uint16_t), (uint8_t *)THERM_TEMP_VAL_val}}};

uint16_t getAttributeIndexByThermHandle(uint16_t attributeHandle)
{
    uint16_t attrIndex = THERM_SERV_NUM_ATTR;
    uint16_t index;

    for (index = 0; index < THERM_SERV_NUM_ATTR; ++index)
    {
        if (therm_handle_table[index] == attributeHandle)
        {
            attrIndex = index;
            break;
        }
    }

    return attrIndex;
}

void handleThermReadEvent(int attrIndex, esp_ble_gatts_cb_param_t *param, esp_gatt_rsp_t *rsp)
{
    if (attrIndex == THERM_TEMP_VAL)
    {
        memset(rsp->attr_value.value, 0, sizeof(rsp->attr_value.value));
        memcpy(rsp->attr_value.value, THERM_TEMP_VAL_val, sizeof(THERM_TEMP_VAL_val));
        rsp->attr_value.len = sizeof(THERM_TEMP_VAL_val);
    }
}

void updateThermValues(float temperature)
{
    int32_t temp = temperature * 100;

    memcpy(THERM_TEMP_VAL_val, &temp, sizeof(temperature));

    ble_gatt_server_notify_indicate(
        therm_handle_table[THERM_TEMP_VAL],
        sizeof(THERM_TEMP_VAL_val),
        THERM_TEMP_VAL_val,
        false);
}