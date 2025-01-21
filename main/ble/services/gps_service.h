#pragma once
#include <stdint.h>

#include "esp_gatt_defs.h"
#include "esp_gatts_api.h"

enum
{
    // Service index
    GPS_SERV,

    GPS_LAT_CHAR,
    GPS_LAT_VAL,
    GPS_LAT_CFG,

    GPS_LON_CHAR,
    GPS_LON_VAL,
    GPS_LON_CFG,

    GPS_ALT_CHAR,
    GPS_ALT_VAL,
    GPS_ALT_CFG,

    GPS_SERV_NUM_ATTR,
};

extern uint16_t gps_handle_table[GPS_SERV_NUM_ATTR];

extern const uint16_t GPS_SERV_uuid;
extern const esp_gatts_attr_db_t gps_serv_gatt_db[GPS_SERV_NUM_ATTR];

uint16_t getAttributeIndexByGpsHandle(uint16_t attributeHandle);
void handleGpsReadEvent(int attrIndex, esp_ble_gatts_cb_param_t *param, esp_gatt_rsp_t *rsp);
void updateGpsValues(float latitude, float longitude, float altitude);