#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "esp_system.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "driver/gpio.h"
#include "wifi.h"
#include "ble/ble_interface.h"
#include "ble/services/wifi_service.h"
#include "mqtt.h"
#include "i2c.h"
#include "sensors/accmeter.h"
#include "esp_log.h"
#include "led.h"

#define BUFFER_SIZE 128
#define TAG "main"

#define BUTTON_GPIO 15

extern int wifiConnected;
extern int configMode;
extern int delay;
int led_state = 0;
int wifiServiceFlag = 0;

void button_isr_handler(void *arg)
{
    configMode = 1;
    wifiServiceFlag = 1;
}

void accelerometer(void *pvParameters)
{
    i2c_master_dev_handle_t dev_handle;
    i2c_master_bus_handle_t bus_handle = (i2c_master_bus_handle_t)pvParameters;
    i2c_add_device(&bus_handle, &dev_handle, ADXL345_I2C_ADDRESS);

    vTaskDelay(100 / portTICK_PERIOD_MS);
    adxl345_init(dev_handle);

    while (1)
    {
        float x = adxl345_read_x(dev_handle);
        float y = adxl345_read_y(dev_handle);
        float z = adxl345_read_z(dev_handle);

        uint8_t x_bytes[4];
        uint8_t y_bytes[4];
        uint8_t z_bytes[4];

        memcpy(x_bytes, &x, sizeof(x));
        memcpy(y_bytes, &y, sizeof(y));
        memcpy(z_bytes, &z, sizeof(z));

        publish_message("/adxl345/x", (const char *)x_bytes, 4);
        publish_message("/adxl345/y", (const char *)y_bytes, 4);
        publish_message("/adxl345/z", (const char *)z_bytes, 4);

        // ESP_LOGI("accel", "X: %.5f, Y: %.5f, Z: %.5f", x, y, z);

        vTaskDelay(67 / portTICK_PERIOD_MS); // 15Hz
    }
}

void app_main(void)
{
    nvs_flash_init();

    i2c_master_bus_handle_t bus_handle;
    i2c_master_init(&bus_handle);

    wifi_init();
    xTaskCreate(ledTask, "blink", 4096, NULL, 4, NULL);

    ble_init();

    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_NEGEDGE, // Trigger on falling edge (button press)
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << BUTTON_GPIO),
        .pull_up_en = GPIO_PULLUP_ENABLE, // Enable internal pull-up resistor
    };
    gpio_config(&io_conf);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_GPIO, button_isr_handler, NULL);

    xTaskCreate(accelerometer, "accelerometer", 4096, (void *)bus_handle, 5, NULL);

    while (wifiConnected == 0)
    {
        if (wifiServiceFlag)
        {
            showWifiService();
            wifiServiceFlag = false;
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    mqtt_init();

    while (1)
    {
        if (wifiServiceFlag)
        {
            showWifiService();
            wifiServiceFlag = false;
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}