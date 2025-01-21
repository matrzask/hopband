#include "gps.h"
#include "driver/uart.h"
#include "esp_log.h"
#include <string.h>
#include <stdlib.h>
#include <freertos/FreeRTOS.h>

#define TAG "GPS"

#define GPS_UART_NUM UART_NUM_2
#define GPS_TX_PIN 17
#define GPS_RX_PIN 16
#define GPS_UART_BAUD_RATE 9600
#define GPS_UART_BUF_SIZE (1024)
#define NMEA_SENTENCE_MAX_LENGTH 82

void gps_uart_init()
{
    const uart_config_t uart_config = {
        .baud_rate = GPS_UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };

    uart_param_config(GPS_UART_NUM, &uart_config);
    uart_set_pin(GPS_UART_NUM, GPS_TX_PIN, GPS_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(GPS_UART_NUM, GPS_UART_BUF_SIZE * 2, 0, 0, NULL, 0);
}

gps_data_t parse_gpgga(char *nmea_sentence)
{
    gps_data_t gps_data = {0};
    char *token = strtok(nmea_sentence, ",");
    int field = 0;

    while (token != NULL)
    {
        switch (field)
        {
        case 2: // Latitude
            gps_data.latitude = atof(token) / 100.0;
            break;
        case 3: // N/S indicator
            if (token[0] == 'S')
                gps_data.latitude = -gps_data.latitude;
            break;
        case 4: // Longitude
            gps_data.longitude = atof(token) / 100.0;
            break;
        case 5: // E/W indicator
            if (token[0] == 'W')
                gps_data.longitude = -gps_data.longitude;
            break;
        case 7: // Number of satellites
            gps_data.satellites = atoi(token);
            break;
        case 9: // Altitude
            gps_data.altitude = atof(token);
            break;
        }
        token = strtok(NULL, ",");
        field++;
    }

    return gps_data;
}

gps_data_t gps_read_data()
{
    static char nmea_buffer[NMEA_SENTENCE_MAX_LENGTH + 1];
    static int nmea_index = 0;
    uint8_t data[GPS_UART_BUF_SIZE];
    int length = uart_read_bytes(GPS_UART_NUM, data, GPS_UART_BUF_SIZE, 100 / portTICK_PERIOD_MS);
    gps_data_t gps_data = {0};

    if (length > 0)
    {
        for (int i = 0; i < length; i++)
        {
            char c = data[i];
            if (c == '\n' || c == '\r')
            {
                if (nmea_index > 0)
                {
                    nmea_buffer[nmea_index] = '\0';
                    // ESP_LOGI("GPS", "Received: %s", nmea_buffer);
                    if (strstr(nmea_buffer, "$GPGGA") != NULL)
                    {
                        gps_data = parse_gpgga(nmea_buffer);
                    }
                    nmea_index = 0;
                }
            }
            else
            {
                if (nmea_index < NMEA_SENTENCE_MAX_LENGTH)
                {
                    nmea_buffer[nmea_index++] = c;
                }
                else
                {
                    nmea_index = 0; // Reset buffer if overflow
                }
            }
        }
    }
    return gps_data;
}