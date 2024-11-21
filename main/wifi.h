#ifndef WIFI_H
#define WIFI_H

#include "esp_wifi.h"
#include <esp_http_client.h>
#include "esp_log.h"

void wifi_init(char *ssid, char *pass);

#endif