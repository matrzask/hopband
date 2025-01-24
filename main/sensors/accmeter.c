#include "accmeter.h"

#define GRAVITY 9.81
#define SCALE_FACTOR_2G (1.0 / 256.0)

void adxl345_init(i2c_master_dev_handle_t dev_handle)
{
    write_reg(dev_handle, ADXL345_POWER_CTL, 0b00001000); // Enable measure mode

    // Set sensitivity to ±2g
    write_reg(dev_handle, ADXL345_DATAFORMAT, 0x08);

    // Enable threshold interrupt on Z axis
    write_reg(dev_handle, ADXL345_THRESH_ACT, 4);                            // Set threshold
    write_reg(dev_handle, ADXL345_ACT_INACT_CTL, ACT_AC_COUPLED | ACT_Z_EN); // Enable activity in Z axis
    write_reg(dev_handle, ADXL345_INT_MAP, 0b00000000);                      // All functions generate INT1
    write_reg(dev_handle, ADXL345_INT_ENABLE, ACTIVITY);                     // Enable activity function
}

float convert_to_m_s2(int16_t raw_value)
{
    float g_value = raw_value * SCALE_FACTOR_2G; // Convert to g
    return g_value * GRAVITY;                    // Convert to m/s²
}

float adxl345_read_x(i2c_master_dev_handle_t dev_handle)
{
    uint8_t data[2];
    read_reg(dev_handle, ADXL345_DATAX, data, 2);
    int16_t x = (data[1] << 8) | data[0];
    return convert_to_m_s2(x);
}

float adxl345_read_y(i2c_master_dev_handle_t dev_handle)
{
    uint8_t data[2];
    read_reg(dev_handle, ADXL345_DATAY, data, 2);
    int16_t y = (data[1] << 8) | data[0];
    return convert_to_m_s2(y);
}

float adxl345_read_z(i2c_master_dev_handle_t dev_handle)
{
    uint8_t data[2];
    read_reg(dev_handle, ADXL345_DATAZ, data, 2);
    int16_t z = (data[1] << 8) | data[0];
    return convert_to_m_s2(z);
}