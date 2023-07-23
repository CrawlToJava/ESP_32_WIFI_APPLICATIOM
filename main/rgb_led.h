#ifndef MAIN_RGB_LED_H
#define MAIN_RGB_LED_H

// define RGB GPIO`s
#define RGB_LED_RED_GPIO 25
#define RGB_LED_GREEN_GPIO 26
#define RGB_LED_BLUE_GPIO 27

// define RGB LED color number of channels
#define RGB_LED_CHANNEL_NUM 3

// RGB LED configuration
typedef struct
{
    int channel;
    int gpio;
    int mode;
    int timer_index;
} ledc_info_t;

// color to indicate WI-FI application has started
void rgb_led_wifi_app_started(void);

// color to indicate http server has started
void rgb_led_http_server_started(void);

// color to indicate that the ESP32 is connected to an access point
void rgb_led_wifi_connected(void);

#endif