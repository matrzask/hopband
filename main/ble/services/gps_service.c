#include "gps_service.h"
#include "../ble_common.h"
#include "../ble_gatts.h"
#include <string.h>
#include "esp_gatts_api.h"
#include "esp_log.h"

#define TAG "BLE-GPS-SERVICE"

uint16_t gps_handle_table[GPS_SERV_NUM_ATTR];

const uint16_t GPS_SERV_uuid = 0x1819;
static const uint16_t GPS_SERV_CHAR_LAT_uuid = 0x2AAE;
static const uint16_t GPS_SERV_CHAR_LON_uuid = 0x2AAF;
static const uint16_t GPS_SERV_CHAR_ALT_uuid = 0x2A6C;

static uint8_t GPS_SERV_CHAR_LAT_val[4] = {0x00, 0x00, 0x00, 0x00};
static uint8_t GPS_SERV_CHAR_LON_val[4] = {0x00, 0x00, 0x00, 0x00};
static uint8_t GPS_SERV_CHAR_ALT_val[4] = {0x00, 0x00, 0x00, 0x00};

const esp_gatts_attr_db_t gps_serv_gatt_db[GPS_SERV_NUM_ATTR] =
    {
        [GPS_SERV] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&primary_service_uuid, ESP_GATT_PERM_READ, sizeof(uint16_t), sizeof(GPS_SERV_uuid), (uint8_t *)&GPS_SERV_uuid}},

        [GPS_LAT_CHAR] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ, CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_notify}},
        [GPS_LAT_VAL] = {{ESP_GATT_RSP_BY_APP}, {ESP_UUID_LEN_16, (uint8_t *)&GPS_SERV_CHAR_LAT_uuid, ESP_GATT_PERM_READ, sizeof(GPS_SERV_CHAR_LAT_val), sizeof(GPS_SERV_CHAR_LAT_val), (uint8_t *)GPS_SERV_CHAR_LAT_val}},
        [GPS_LAT_CFG] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_client_config_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, sizeof(uint16_t), sizeof(uint16_t), (uint8_t *)GPS_SERV_CHAR_LAT_val}},

        [GPS_LON_CHAR] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ, CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_notify}},
        [GPS_LON_VAL] = {{ESP_GATT_RSP_BY_APP}, {ESP_UUID_LEN_16, (uint8_t *)&GPS_SERV_CHAR_LON_uuid, ESP_GATT_PERM_READ, sizeof(GPS_SERV_CHAR_LON_val), sizeof(GPS_SERV_CHAR_LON_val), (uint8_t *)GPS_SERV_CHAR_LON_val}},
        [GPS_LON_CFG] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_client_config_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, sizeof(uint16_t), sizeof(uint16_t), (uint8_t *)GPS_SERV_CHAR_LON_val}},

        [GPS_ALT_CHAR] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ, CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_notify}},
        [GPS_ALT_VAL] = {{ESP_GATT_RSP_BY_APP}, {ESP_UUID_LEN_16, (uint8_t *)&GPS_SERV_CHAR_ALT_uuid, ESP_GATT_PERM_READ, sizeof(GPS_SERV_CHAR_ALT_val), sizeof(GPS_SERV_CHAR_ALT_val), (uint8_t *)GPS_SERV_CHAR_ALT_val}},
        [GPS_ALT_CFG] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_client_config_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, sizeof(uint16_t), sizeof(uint16_t), (uint8_t *)GPS_SERV_CHAR_ALT_val}}};

uint16_t getAttributeIndexByGpsHandle(uint16_t attributeHandle)
{
    // Get the attribute index in the attribute table by the returned handle

    uint16_t attrIndex = GPS_SERV_NUM_ATTR;
    uint16_t index;

    for (index = 0; index < GPS_SERV_NUM_ATTR; ++index)
    {
        if (gps_handle_table[index] == attributeHandle)
        {
            attrIndex = index;
            break;
        }
    }

    return attrIndex;
}

void handleGpsReadEvent(int attrIndex, esp_ble_gatts_cb_param_t *param, esp_gatt_rsp_t *rsp)
{
    switch (attrIndex)
    {
    case GPS_LAT_VAL:
        memset(rsp->attr_value.value, 0, sizeof(rsp->attr_value.value));
        memcpy(rsp->attr_value.value, GPS_SERV_CHAR_LAT_val, sizeof(GPS_SERV_CHAR_LAT_val));
        rsp->attr_value.len = sizeof(GPS_SERV_CHAR_LAT_val);
        break;

    case GPS_LON_VAL:
        memset(rsp->attr_value.value, 0, sizeof(rsp->attr_value.value));
        memcpy(rsp->attr_value.value, GPS_SERV_CHAR_LON_val, sizeof(GPS_SERV_CHAR_LON_val));
        rsp->attr_value.len = sizeof(GPS_SERV_CHAR_LON_val);
        break;

    case GPS_ALT_VAL:
        memset(rsp->attr_value.value, 0, sizeof(rsp->attr_value.value));
        memcpy(rsp->attr_value.value, GPS_SERV_CHAR_ALT_val, sizeof(GPS_SERV_CHAR_ALT_val));
        rsp->attr_value.len = sizeof(GPS_SERV_CHAR_ALT_val);
        break;
    }
}

void updateGpsValues(float latitude, float longitude, float altitude)
{
    int32_t lat = latitude * 10000;
    int32_t lon = longitude * 10000;
    int32_t alt = altitude * 100;

    // Update the GPS values in the GATT database
    memcpy(GPS_SERV_CHAR_LAT_val, &lat, sizeof(latitude));
    memcpy(GPS_SERV_CHAR_LON_val, &lon, sizeof(longitude));
    memcpy(GPS_SERV_CHAR_ALT_val, &alt, sizeof(altitude));

    // Send notifications
    ble_gatt_server_notify_indicate(
        gps_handle_table[GPS_LAT_VAL],
        sizeof(GPS_SERV_CHAR_LAT_val),
        GPS_SERV_CHAR_LAT_val,
        false);

    ble_gatt_server_notify_indicate(
        gps_handle_table[GPS_LON_VAL],
        sizeof(GPS_SERV_CHAR_LON_val),
        GPS_SERV_CHAR_LON_val,
        false);

    ble_gatt_server_notify_indicate(
        gps_handle_table[GPS_ALT_VAL],
        sizeof(GPS_SERV_CHAR_ALT_val),
        GPS_SERV_CHAR_ALT_val,
        false);
}