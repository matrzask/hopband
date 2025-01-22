#pragma once
#include <stdint.h>

uint8_t *get_mac_address();
void wifi_init();
void set_wifi_credentials(char *ssid, char *pass);