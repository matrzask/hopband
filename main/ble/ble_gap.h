#pragma once
#include "esp_err.h"

#define ADV_CONFIG_FLAG (1 << 0)
#define SCAN_RSP_CONFIG_FLAG (1 << 1)

void ble_gap_setAdvertisingData(void);
void ble_gap_startAdvertising(void);
esp_err_t ble_gap_init(void);