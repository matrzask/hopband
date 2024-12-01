#pragma once
#include <stdint.h>

#include "esp_gatt_defs.h"
#include "esp_gatts_api.h"

enum
{
    // Service index
    BATTERY_SERV,

    BATTERY_INFO_CHAR,
    BATTERY_INFO_VAL,
    // BATTERY_INFO_DESCR,
    BATTERY_INFO_CFG,

    BATTERY_SERV_NUM_ATTR,
};

extern uint16_t battery_handle_table[BATTERY_SERV_NUM_ATTR];

extern const uint16_t BATTERY_SERV_uuid;
extern const esp_gatts_attr_db_t battery_serv_gatt_db[BATTERY_SERV_NUM_ATTR];

uint16_t getAttributeIndexByBatteryHandle(uint16_t attributeHandle);
void handleBatteryReadEvent(int attrIndex, esp_ble_gatts_cb_param_t *param, esp_gatt_rsp_t *rsp);
void updateBatteryValue(uint8_t batteryValue);