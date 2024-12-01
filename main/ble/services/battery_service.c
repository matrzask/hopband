#include "battery_service.h"
#include "../ble_common.h"
#include "../ble_gatts.h"
#include <string.h>
#include "esp_gatts_api.h"
#include "esp_log.h"

#define TAG "BLE-BATTERY-SERVICE"

uint16_t battery_handle_table[BATTERY_SERV_NUM_ATTR];

const uint16_t BATTERY_SERV_uuid = 0x180F;
static const uint16_t BATTERY_SERV_CHAR_info_uuid = 0x2A19;

// static const uint8_t BATTERY_SERV_descr[] = "Battery Service";
// static const uint8_t BATTERY_SERV_CHAR_info_descr[] = "Battery Value";

static uint8_t BATTERY_SERV_CHAR_info_ccc[2] = {0x00, 0x00};

static uint8_t BATTERY_SERV_CHAR_info_val[1] = {0x00};

const esp_gatts_attr_db_t battery_serv_gatt_db[BATTERY_SERV_NUM_ATTR] =
    {
        [BATTERY_SERV] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&primary_service_uuid, ESP_GATT_PERM_READ, sizeof(uint16_t), sizeof(BATTERY_SERV_uuid), (uint8_t *)&BATTERY_SERV_uuid}},

        [BATTERY_INFO_CHAR] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ, CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_notify}},
        [BATTERY_INFO_VAL] = {{ESP_GATT_RSP_BY_APP}, {ESP_UUID_LEN_16, (uint8_t *)&BATTERY_SERV_CHAR_info_uuid, ESP_GATT_PERM_READ, sizeof(uint16_t), 0, NULL}},
        //[BATTERY_INFO_DESCR] = {{ESP_GATT_RSP_BY_APP}, {ESP_UUID_LEN_16, (uint8_t *)&character_description, ESP_GATT_PERM_READ, CHAR_DECLARATION_SIZE, 0, NULL}},
        [BATTERY_INFO_CFG] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_client_config_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, sizeof(uint16_t), sizeof(BATTERY_SERV_CHAR_info_ccc), (uint8_t *)BATTERY_SERV_CHAR_info_ccc}},
};

uint16_t getAttributeIndexByBatteryHandle(uint16_t attributeHandle)
{
    // Get the attribute index in the attribute table by the returned handle

    uint16_t attrIndex = BATTERY_SERV_NUM_ATTR;
    uint16_t index;

    for (index = 0; index < BATTERY_SERV_NUM_ATTR; ++index)
    {
        if (battery_handle_table[index] == attributeHandle)
        {
            attrIndex = index;
            break;
        }
    }

    return attrIndex;
}

void handleBatteryReadEvent(int attrIndex, esp_ble_gatts_cb_param_t *param, esp_gatt_rsp_t *rsp)
{
    switch (attrIndex)
    {
    // Characteristic read values
    case BATTERY_INFO_VAL:
        memset(rsp->attr_value.value, 0, sizeof(rsp->attr_value.value));
        memcpy(rsp->attr_value.value, BATTERY_SERV_CHAR_info_val, sizeof(BATTERY_SERV_CHAR_info_val));
        rsp->attr_value.len = sizeof(BATTERY_SERV_CHAR_info_val);
        break;

        // Characteristic descriptions
        /*case BATTERY_INFO_DESCR:
            memset(rsp->attr_value.value, 0, sizeof(rsp->attr_value.value));
            memcpy(rsp->attr_value.value, BATTERY_SERV_CHAR_info_descr, sizeof(BATTERY_SERV_CHAR_info_descr));
            rsp->attr_value.len = sizeof(BATTERY_SERV_CHAR_info_descr);
            break;*/
    }
}

void updateBatteryValue(uint8_t batteryValue)
{
    // Update the heart rate value in the GATT database
    BATTERY_SERV_CHAR_info_val[0] = batteryValue;

    // Send notification
    ble_gatt_server_notify_indicate(
        battery_handle_table[BATTERY_INFO_VAL],
        sizeof(BATTERY_SERV_CHAR_info_val),
        BATTERY_SERV_CHAR_info_val,
        false);
}