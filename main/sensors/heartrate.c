#include "heartrate.h"
#include "../i2c.h"

void max30102_init(i2c_master_dev_handle_t dev_handle, max_config *configuration)
{
    write_reg(dev_handle, REG_INTR_ENABLE_2, configuration->data2);
    write_reg(dev_handle, REG_FIFO_WR_PTR, configuration->data3);
    write_reg(dev_handle, REG_OVF_COUNTER, configuration->data4);
    write_reg(dev_handle, REG_FIFO_RD_PTR, configuration->data5);
    write_reg(dev_handle, REG_FIFO_CONFIG, configuration->data6);
    write_reg(dev_handle, REG_MODE_CONFIG, configuration->data7);
    write_reg(dev_handle, REG_SPO2_CONFIG, configuration->data8);
    write_reg(dev_handle, REG_LED1_PA, configuration->data9);
    write_reg(dev_handle, REG_LED2_PA, configuration->data10);
    write_reg(dev_handle, REG_PILOT_PA, configuration->data11);
    write_reg(dev_handle, REG_MULTI_LED_CTRL1, configuration->data12);
    write_reg(dev_handle, REG_MULTI_LED_CTRL2, configuration->data13);
}

void read_max30102_fifo(i2c_master_dev_handle_t dev_handle, int32_t *red_data, int32_t *ir_data)
{
    uint8_t un_temp[6];
    uint8_t fifo_reg = REG_FIFO_DATA;

    read_reg(dev_handle, fifo_reg, un_temp, 6);
    *red_data += un_temp[0] << 16;
    *red_data += un_temp[1] << 8;
    *red_data += un_temp[2];

    *ir_data += un_temp[3] << 16;
    *ir_data += un_temp[4] << 8;
    *ir_data += un_temp[5];
}