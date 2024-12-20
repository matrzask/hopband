#pragma once
#include "driver/i2c_master.h"

void i2c_master_init(i2c_master_bus_handle_t *bus_handle);
void i2c_add_device(i2c_master_bus_handle_t *bus_handle, i2c_master_dev_handle_t *dev_handle, uint8_t addr);
esp_err_t write_reg(i2c_master_dev_handle_t dev_handle, uint8_t reg_addr, uint8_t data);
esp_err_t read_reg(i2c_master_dev_handle_t dev_handle, uint8_t reg_addr, uint8_t *data, size_t len);