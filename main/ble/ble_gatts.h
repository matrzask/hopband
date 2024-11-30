#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "esp_err.h"

esp_err_t ble_gatt_server_init();

esp_err_t ble_gatt_server_notify_indicate(uint16_t attr_handle, uint16_t value_len, uint8_t *value, bool need_confirm);