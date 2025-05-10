idf_component_register(SRCS "wifi.c" "main.c" "nvstorage.c" "i2c.c" "led.c" "mqtt.c"
                            "sensors/accmeter.c"
                            "ble/ble_gap.c" "ble/ble_gatts.c" "ble/ble_interface.c" "ble/ble_common.c"
                           "ble/services/wifi_service.c"
                    INCLUDE_DIRS ".")