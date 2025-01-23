idf_component_register(SRCS "wifi.c" "main.c" "nvstorage.c" "i2c.c" "led.c" "mqtt.c"
                            "sensors/gps.c" "sensors/thermistor.c" "sensors/heartrate.c" "sensors/accmeter.c"
                            "ble/ble_gap.c" "ble/ble_gatts.c" "ble/ble_interface.c" "ble/ble_common.c"
                            "ble/services/gps_service.c" "ble/services/wifi_service.c" "ble/services/thermistor_service.c" "ble/services/heartrate_service.c" "ble/services/thermistor_service.c"
                    INCLUDE_DIRS ".")