#include "rgb_led.h"
#include "driver/ledc.h"
#include <stdbool.h>

ledc_info_t ledc_ch[RGB_LED_CHANNEL_NUM];

// handle for rgb_led_pwm_init
bool g_pwm_init_handle = false;

/**
 * Initialize the RGB LED settings per channel, including
 * the GPIO for each channel, mode and timer configuration
 */
void rgb_led_pwm_init(void)
{
    int rgb_ch;

    // RGB LED RED initialization
    ledc_ch[0].gpio = RGB_LED_RED_GPIO;
    ledc_ch[0].channel = LEDC_CHANNEL_0;
    ledc_ch[0].mode = LEDC_HIGH_SPEED_MODE;
    ledc_ch[0].timer_index = LEDC_TIMER_0;

    // RGB LED GREEN initialization
    ledc_ch[1].gpio = RGB_LED_GREEN_GPIO;
    ledc_ch[1].channel = LEDC_CHANNEL_1;
    ledc_ch[1].mode = LEDC_HIGH_SPEED_MODE;
    ledc_ch[1].timer_index = LEDC_TIMER_0;

    // RGB LED BLUE initialization
    ledc_ch[2].gpio = RGB_LED_BLUE_GPIO;
    ledc_ch[2].channel = LEDC_CHANNEL_2;
    ledc_ch[2].mode = LEDC_HIGH_SPEED_MODE;
    ledc_ch[2].timer_index = LEDC_TIMER_0;

    // Configure timer zero
    ledc_timer_config_t timer_config = {
        .timer_num = LEDC_TIMER_0,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_8_BIT,
        .freq_hz = 100};

    ledc_timer_config(&timer_config);

    // Configure chanells
    for (rgb_ch = 0; rgb_ch < RGB_LED_CHANNEL_NUM; rgb_ch++)
    {
        ledc_channel_config_t channel_config = {
            .channel = ledc_ch[rgb_ch].channel,
            .duty = 0,
            .hpoint = 0,
            .gpio_num = ledc_ch[rgb_ch].gpio,
            .intr_type = LEDC_INTR_DISABLE,
            .speed_mode = ledc_ch[rgb_ch].mode,
            .timer_sel = ledc_ch[rgb_ch].timer_index};

        ledc_channel_config(&channel_config);
    }

    g_pwm_init_handle = true;
}

/**
 * Sets the RGB color
 */
static void rgb_led_set_color(uint8_t red, uint8_t green, uint8_t blue)
{
    uint8_t colors[3] = {red, green, blue};
    for (int i = 0; i < RGB_LED_CHANNEL_NUM; i++)
    {
        ledc_set_duty(ledc_ch[i].mode, ledc_ch[i].channel, colors[i]);
        ledc_update_duty(ledc_ch[i].mode, ledc_ch[i].channel);
    }
}

void rgb_led_wifi_app_started(void)
{
    if (g_pwm_init_handle != true)
    {
        rgb_led_pwm_init();
    }
    rgb_led_set_color(255, 102, 255);
}

void rgb_led_http_server_started(void)
{
    if (g_pwm_init_handle != true)
    {
        rgb_led_pwm_init();
    }
    rgb_led_set_color(204, 255, 51);
}

void rgb_led_wifi_connected(void)
{
    if (g_pwm_init_handle != true)
    {
        rgb_led_pwm_init();
    }
    rgb_led_set_color(0, 255, 153);
}
