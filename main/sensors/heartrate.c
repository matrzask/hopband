#include "heartrate.h"
#include "../i2c.h"
#include <math.h>

#define MINIMUM_RATIO 0.3
#define DELAY 40
#define BUFFER_SIZE 128

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

float get_max30102_temp(i2c_master_dev_handle_t dev_handle)
{
    uint8_t int_temp;
    uint8_t decimal_temp;
    float temp = 0;
    write_reg(dev_handle, REG_TEMP_CONFIG, 1);
    read_reg(dev_handle, REG_TEMP_INTR, &int_temp, 1);
    read_reg(dev_handle, REG_TEMP_FRAC, &decimal_temp, 1);
    temp = (int_temp + ((float)decimal_temp / 10));
    return temp;
}

void remove_dc_part(int32_t *ir_buffer, int32_t *red_buffer, uint64_t *ir_mean, uint64_t *red_mean)
{
    *ir_mean = 0;
    *red_mean = 0;
    for (int i = 0; i < BUFFER_SIZE; i++)
    {
        *ir_mean += ir_buffer[i];
        *red_mean += red_buffer[i];
    }

    *ir_mean = *ir_mean / (BUFFER_SIZE);
    *red_mean = *red_mean / (BUFFER_SIZE);

    for (int i = 0; i < BUFFER_SIZE; i++)
    {
        red_buffer[i] = red_buffer[i] - *red_mean;
        ir_buffer[i] = ir_buffer[i] - *ir_mean;
    }
}

int64_t sum_of_elements(int32_t *data)
{
    int64_t sum = 0;
    for (int i = 0; i < BUFFER_SIZE; i++)
    {
        sum += data[i];
    }
    return sum;
}

double sum_of_xy_elements(int32_t *data)
{
    double sum_xy = 0;
    double time = 0;
    for (int i = 0; i < BUFFER_SIZE; i++)
    {
        sum_xy += (data[i] * time);
        time += DELAY / 1000.0;
    }
    return sum_xy;
}

double sum_of_squared_elements(int32_t *data)
{
    double sum_squared = 0;
    int time = 0;
    for (int i = 0; i < BUFFER_SIZE; i++)
    {
        sum_squared += (data[i] * data[i]);
        time += DELAY / 1000.0;
    }
    return sum_squared;
}

double sum_x2()
{
    float incremen = (DELAY / 1000.0);
    double result = 0.0;
    double squared_values = 0;
    float temp = 0;

    for (int i = 0; i < BUFFER_SIZE; i++)
    {
        squared_values = temp * temp;
        temp += incremen;
        result += squared_values;
    }
    return result;
}

void calculate_linear_regression(double *angular_coef, double *linear_coef, int32_t *data)
{
    int64_t sum_of_y = sum_of_elements(data);
    double sum_of_x = 325.12;
    double sum_of_x2 = sum_x2();
    double sum_of_xy = sum_of_xy_elements(data);
    double sum_of_x_squared = (sum_of_x * sum_of_x);

    double temp = (sum_of_xy - (sum_of_x * sum_of_y) / BUFFER_SIZE);
    double temp2 = (sum_of_x2 - (sum_of_x_squared / BUFFER_SIZE));

    *angular_coef = temp / temp2;
    *linear_coef = ((sum_of_y / BUFFER_SIZE) - (*angular_coef * (sum_of_x / BUFFER_SIZE)));
}

void remove_trend_line(int32_t *buffer)
{
    double a = 0;
    double b = 0;

    calculate_linear_regression(&a, &b, buffer);

    double time = 0;
    for (int i = 0; i < BUFFER_SIZE; i++)
    {
        buffer[i] = ((buffer[i] + (-a * time)) - b);
        time += DELAY / 1000.0;
    }
}

double auto_correlation_function(int32_t *data, int32_t lag)
{
    double sum = 0;
    double result = 0;
    for (int i = 0; i < (BUFFER_SIZE - lag); i++)
    {
        sum += ((data[i]) * (data[i + lag]));
    }
    result = sum / BUFFER_SIZE;
    return result;
}

int calculate_heart_rate(int32_t *ir_data)
{
    double auto_correlation_result;
    double result = -1;
    double auto_correlation_0 = auto_correlation_function(ir_data, 0);
    double biggest_value = 0;
    int biggest_value_index = 0;
    double division;

    for (float i = 0; i < 125; i++)
    {
        auto_correlation_result = auto_correlation_function(ir_data, i);
        division = auto_correlation_result / auto_correlation_0;

        if (i > 10)
        {
            if (division > MINIMUM_RATIO)
            {
                if (biggest_value < division)
                {
                    biggest_value = division;
                    biggest_value_index = i;
                    continue;
                }

                result = ((1 / (biggest_value_index * (DELAY / 1000.0))) * 60);
            }
        }
    }
    return (int)result;
}

double rms_value(int32_t *data)
{
    double result = 0;
    int32_t sum = 0;
    for (int i = 0; i < BUFFER_SIZE; i++)
    {
        sum += (data[i] * data[i]);
    }
    result = sqrt(sum / BUFFER_SIZE);
    return result;
}

double spo2_measurement(int32_t *ir_data, int32_t *red_data, uint64_t ir_mean, uint64_t red_mean)
{
    double Z = 0;
    double SpO2;
    double ir_rms = rms_value(ir_data);
    double red_rms = rms_value(red_data);

    Z = (red_rms / red_mean) / (ir_rms / ir_mean);
    SpO2 = 104 - 17 * Z;

    return SpO2;
}