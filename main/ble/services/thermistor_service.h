#pragma once
#include <stdint.h>

#include "esp_gatt_defs.h"
#include "esp_gatts_api.h"

enum
{
    // Service index
    THERM_SERV,

    THERM_TEMP_CHAR,
    THERM_TEMP_VAL,
    THERM_TEMP_CFG,

    THERM_SERV_NUM_ATTR,
};

extern uint16_t therm_handle_table[THERM_SERV_NUM_ATTR];

extern const uint16_t THERM_SERV_uuid;
extern const esp_gatts_attr_db_t therm_serv_gatt_db[THERM_SERV_NUM_ATTR];

uint16_t getAttributeIndexByThermHandle(uint16_t attributeHandle);
void handleThermReadEvent(int attrIndex, esp_ble_gatts_cb_param_t *param, esp_gatt_rsp_t *rsp);
void updateThermValues(float temperature);