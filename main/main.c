#include "main.h"

void app_main(void)
{
    while (1)
    {
        rgb_led_http_server_started();
        vTaskDelay(300 / portTICK_PERIOD_MS);
        rgb_led_wifi_app_started();
        vTaskDelay(300 / portTICK_PERIOD_MS);
        rgb_led_wifi_connected();
        vTaskDelay(300 / portTICK_PERIOD_MS);
    }
}
