#pragma once

void mqtt_init(void);
void publish_message(const char *topic, const char *data, int len);