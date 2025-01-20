#include "led.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"

#define RED_LED_GPIO 18
#define GREEN_LED_GPIO 5
#define BLINK_PERIOD 1000

int led_color = 0;

extern int wifiConnected;
extern int configMode;

void ledTask(void *pvParameters)
{
    gpio_reset_pin(RED_LED_GPIO);
    gpio_set_direction(RED_LED_GPIO, GPIO_MODE_OUTPUT);
    gpio_reset_pin(GREEN_LED_GPIO);
    gpio_set_direction(GREEN_LED_GPIO, GPIO_MODE_OUTPUT);

    int count = 0;
    int led_state = 0;
    while (1)
    {
        if (configMode)
        {
            gpio_set_level(RED_LED_GPIO, 0);
            count = (count + 1) % 10;
            if (count == 0)
                led_state = !led_state;
            gpio_set_level(GREEN_LED_GPIO, led_state);
            vTaskDelay(BLINK_PERIOD / portTICK_PERIOD_MS / 10);
        }
        else if (wifiConnected)
        {
            gpio_set_level(GREEN_LED_GPIO, 1);
            gpio_set_level(RED_LED_GPIO, 0);
            vTaskDelay(BLINK_PERIOD / portTICK_PERIOD_MS / 10);
        }
        else
        {
            gpio_set_level(GREEN_LED_GPIO, 0);
            count = (count + 1) % 10;
            if (count == 0)
                led_state = !led_state;
            gpio_set_level(RED_LED_GPIO, led_state);
            vTaskDelay(BLINK_PERIOD / portTICK_PERIOD_MS / 10);
        }
    }
}