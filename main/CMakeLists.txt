idf_component_register(SRCS "wifi.c" "main.c" "nvstorage.c" "i2c.c" "led.c" "mqtt.c"
                            "sensors/accmeter.c"
                            "ble/ble_gap.c" "ble/ble_gatts.c" "ble/ble_interface.c" "ble/ble_common.c"
                           "ble/services/wifi_service.c"
                           "LVGL_Driver/LVGL_Driver.c"
                            "LCD_Driver/ST7789.c" "LCD_Driver/Vernon_ST7789T/Vernon_ST7789T.c"
                    INCLUDE_DIRS "." "./LCD_Driver" "./LVGL_Driver" "./LCD_Driver/Vernon_ST7789T")