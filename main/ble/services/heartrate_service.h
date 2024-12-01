#pragma once
#include <stdint.h>

#include "esp_gatt_defs.h"
#include "esp_gatts_api.h"

enum
{
    // Service index
    HEARTRATE_SERV,

    HEARTRATE_INFO_CHAR,
    HEARTRATE_INFO_VAL,
    HEARTRATE_INFO_DESCR,
    HEARTRATE_INFO_CFG,

    HEARTRATE_SERV_NUM_ATTR,
};

extern uint16_t heartrate_handle_table[HEARTRATE_SERV_NUM_ATTR];

extern const uint16_t HEARTRATE_SERV_uuid;
extern const esp_gatts_attr_db_t heartrate_serv_gatt_db[HEARTRATE_SERV_NUM_ATTR];

uint16_t getAttributeIndexByHeartrateHandle(uint16_t attributeHandle);
void handleHeartrateReadEvent(int attrIndex, esp_ble_gatts_cb_param_t *param, esp_gatt_rsp_t *rsp);