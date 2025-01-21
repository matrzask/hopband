#pragma once

typedef struct
{
    float latitude;
    float longitude;
    int satellites;
    float altitude;
} gps_data_t;

void gps_uart_init();
gps_data_t gps_read_data();