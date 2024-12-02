#pragma once
#include <stdint.h>

#include "esp_gatt_defs.h"
#include "esp_gatts_api.h"

enum
{
    // Service index
    WIFI_SERV,

    WIFI_SSID_CHAR,
    WIFI_SSID_VAL,
    WIFI_SSID_DESCR,
    WIFI_SSID_CFG,

    WIFI_PASS_CHAR,
    WIFI_PASS_VAL,
    WIFI_PASS_DESCR,
    WIFI_PASS_CFG,

    WIFI_SERV_NUM_ATTR,
};

extern uint16_t wifi_handle_table[WIFI_SERV_NUM_ATTR];

extern const uint16_t WIFI_SERV_uuid;
extern const esp_gatts_attr_db_t wifi_serv_gatt_db[WIFI_SERV_NUM_ATTR];

uint16_t getAttributeIndexByWifiHandle(uint16_t attributeHandle);
void handleWifiReadEvent(int attrIndex, esp_ble_gatts_cb_param_t *param, esp_gatt_rsp_t *rsp);
void handleWifiWriteEvent(int attrIndex, const uint8_t *char_val_p, uint16_t char_val_len);