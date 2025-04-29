#include "wm_drv_gpio.h"
#include "wm_dt.h"
#include "wm_drv_pwm.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LED_R_PIN WM_GPIO_NUM_16 //pwm channel 1
#define LED_G_PIN WM_GPIO_NUM_17 //pwm channel 0
#define LED_B_PIN WM_GPIO_NUM_18 //pwm channel 2

#define PWM_DUTY_MAX    255
#define PWM_DUTY_MIN    1

static wm_device_t *pwm_device = NULL;
static int switch_state = 0;

void led_brightness(int brightness)
{
    wm_drv_pwm_set_channel_duty(pwm_device, WM_PWM_CHANNEL_1, PWM_DUTY_MAX - (brightness * PWM_DUTY_MAX / 100));
    wm_drv_pwm_set_channel_duty(pwm_device, WM_PWM_CHANNEL_0, PWM_DUTY_MAX - (brightness * PWM_DUTY_MAX / 100));
    wm_drv_pwm_set_channel_duty(pwm_device, WM_PWM_CHANNEL_2, PWM_DUTY_MAX - (brightness * PWM_DUTY_MAX / 100));

    if (!brightness)
        switch_state = 0;
    else
        switch_state = 1;
}

void led_on(void)
{
#if 0
    wm_drv_gpio_data_reset(LED_R_PIN);
    wm_drv_gpio_data_reset(LED_G_PIN);
    wm_drv_gpio_data_reset(LED_B_PIN);
#else
    led_brightness(100);
    switch_state = 1;
#endif
}

void led_off(void)
{
#if 0
    wm_drv_gpio_data_set(LED_R_PIN);
    wm_drv_gpio_data_set(LED_G_PIN);
    wm_drv_gpio_data_set(LED_B_PIN);
#else
    led_brightness(0);
    switch_state = 0;
#endif
}

int led_state(void)
{
    return switch_state;//wm_drv_gpio_data_get(LED_R_PIN) == 0 ? 1 : 0;
}

void led_red(int brightness)
{
    wm_drv_pwm_set_channel_duty(pwm_device, WM_PWM_CHANNEL_1, PWM_DUTY_MAX - (brightness * PWM_DUTY_MAX / 100));

    if (!brightness)
        switch_state = 0;
    else
        switch_state = 1;
}

void led_green(int brightness)
{
    wm_drv_pwm_set_channel_duty(pwm_device, WM_PWM_CHANNEL_0, PWM_DUTY_MAX - (brightness * PWM_DUTY_MAX / 100));

    if (!brightness)
        switch_state = 0;
    else
        switch_state = 1;
}

void led_blue(int brightness)
{
    wm_drv_pwm_set_channel_duty(pwm_device, WM_PWM_CHANNEL_2, PWM_DUTY_MAX - (brightness * PWM_DUTY_MAX / 100));

    if (!brightness)
        switch_state = 0;
    else
        switch_state = 1;
}

void led_breathing(int color)
{
    if (color)
    {
        uint8_t  channel = 0;
        uint16_t duty_red = PWM_DUTY_MIN;
        uint8_t  red_reverse = 0;
        uint16_t duty_green = (PWM_DUTY_MAX - PWM_DUTY_MIN) / 2;
        uint8_t  green_reverse = 0;
        uint16_t duty_blue = PWM_DUTY_MAX;
        uint8_t  blue_reverse = 0;

        //pwm_demo_start(0, 100, duty_blue, 4, 0);
        //pwm_demo_start(1, 100, duty_green, 4, 0);
        //pwm_demo_start(2, 100, duty_red, 4, 0);
        wm_drv_pwm_set_channel_duty(pwm_device, WM_PWM_CHANNEL_0, duty_green);
        wm_drv_pwm_set_channel_duty(pwm_device, WM_PWM_CHANNEL_1, duty_red);
        wm_drv_pwm_set_channel_duty(pwm_device, WM_PWM_CHANNEL_2, duty_blue);

        while (1)
        {
            switch (channel)
            {
                case 0:
                {
                    if (blue_reverse)
                    {
                        duty_blue--;
                        if (duty_blue < PWM_DUTY_MIN)
                        {
                            blue_reverse = 0;
                            duty_blue = PWM_DUTY_MIN;
                        }
                    }
                    else
                    {
                        duty_blue++;
                        if (duty_blue > PWM_DUTY_MAX)
                        {
                            blue_reverse = 1;
                            duty_blue = PWM_DUTY_MAX;
                        }
                    }

                    //tls_pwm_duty_set(channel, duty_blue);
                    wm_drv_pwm_set_channel_duty(pwm_device, WM_PWM_CHANNEL_2, duty_blue);
                    break;
                }
                case 1:
                {
                    if (green_reverse)
                    {
                        duty_green--;
                        if (duty_green < PWM_DUTY_MIN)
                        {
                            green_reverse = 0;
                            duty_green = PWM_DUTY_MIN;
                        }
                    }
                    else
                    {
                        duty_green++;
                        if (duty_green > PWM_DUTY_MAX)
                        {
                            green_reverse = 1;
                            duty_green = PWM_DUTY_MAX;
                        }
                    }

                    //tls_pwm_duty_set(channel, duty_green);
                    wm_drv_pwm_set_channel_duty(pwm_device, WM_PWM_CHANNEL_0, duty_green);
                    break;
                }
                case 2:
                {
                    if (red_reverse)
                    {
                        duty_red--;
                        if (duty_red < PWM_DUTY_MIN)
                        {
                            red_reverse = 0;
                            duty_red = PWM_DUTY_MIN;
                        }
                    }
                    else
                    {
                        duty_red++;
                        if (duty_red > PWM_DUTY_MAX)
                        {
                            red_reverse = 1;
                            duty_red = PWM_DUTY_MAX;
                        }
                    }

                    //tls_pwm_duty_set(channel, duty_red);
                    wm_drv_pwm_set_channel_duty(pwm_device, WM_PWM_CHANNEL_1, duty_red);
                    break;
                }
                default:
                {
                    break;
                }
            }

            //printf("channel: %hhu, red duty: %hhu, green duty: %hhu, blue duty: %hhu\n", channel, duty_red, duty_green, duty_blue);

            if (++channel > 2) channel = 0;

            wm_os_internal_time_delay(2);
        }

    }
    else
    {

        uint16_t duty = PWM_DUTY_MIN;
        uint8_t  reverse = 0;

        //pwm_demo_start(0, 100, duty, 4, 0);
        //pwm_demo_start(1, 100, duty, 4, 0);
        //pwm_demo_start(2, 100, duty, 4, 0);
        wm_drv_pwm_set_channel_duty(pwm_device, WM_PWM_CHANNEL_0, duty);
        wm_drv_pwm_set_channel_duty(pwm_device, WM_PWM_CHANNEL_1, duty);
        wm_drv_pwm_set_channel_duty(pwm_device, WM_PWM_CHANNEL_2, duty);

        while (1)
        {
            if (reverse)
            {
                duty--;
                if (duty < PWM_DUTY_MIN)
                {
                    reverse = 0;
                    duty = PWM_DUTY_MIN;
                }
            }
            else
            {
                duty++;
                if (duty > PWM_DUTY_MAX)
                {
                    reverse = 1;
                    duty = PWM_DUTY_MAX;
                }
            }

            //tls_pwm_duty_set(0, duty);
            //tls_pwm_duty_set(1, duty);
            //tls_pwm_duty_set(2, duty);
            wm_drv_pwm_set_channel_duty(pwm_device, WM_PWM_CHANNEL_0, duty);
            wm_drv_pwm_set_channel_duty(pwm_device, WM_PWM_CHANNEL_1, duty);
            wm_drv_pwm_set_channel_duty(pwm_device, WM_PWM_CHANNEL_2, duty);

            //printf("duty: %hhu\n", duty);

            wm_os_internal_time_delay(6);
        }
    }
}

void led_init(void)
{
#if 0
    wm_drv_gpio_iomux_func_sel(LED_R_PIN, WM_GPIO_IOMUX_FUN5);
    wm_drv_gpio_set_dir(LED_R_PIN, WM_GPIO_DIR_OUTPUT);
    wm_drv_gpio_set_pullmode(LED_R_PIN, WM_GPIO_FLOAT);

    wm_drv_gpio_iomux_func_sel(LED_G_PIN, WM_GPIO_IOMUX_FUN5);
    wm_drv_gpio_set_dir(LED_G_PIN, WM_GPIO_DIR_OUTPUT);
    wm_drv_gpio_set_pullmode(LED_G_PIN, WM_GPIO_FLOAT);

    wm_drv_gpio_iomux_func_sel(LED_B_PIN, WM_GPIO_IOMUX_FUN5);
    wm_drv_gpio_set_dir(LED_B_PIN, WM_GPIO_DIR_OUTPUT);
    wm_drv_gpio_set_pullmode(LED_B_PIN, WM_GPIO_FLOAT);

    led_off();
#else
    wm_drv_pwm_channel_cfg_t cfg = {0};
    pwm_device = wm_drv_pwm_init("pwm");


    cfg.mode    = WM_PWM_OUT_INDPT;
    cfg.clkdiv  = WM_PWM_CLKDIV_DEFAULT;
    cfg.period_cycle = WM_PWM_MAX_PERIOD;
    cfg.duty_cycle = PWM_DUTY_MAX;
    cfg.autoload   = true;

    if (pwm_device)
    {
        cfg.channel = WM_PWM_CHANNEL_1;
        wm_drv_pwm_set_channel_duty(pwm_device, cfg.channel, cfg.duty_cycle);
        wm_drv_pwm_channel_init(pwm_device, &cfg);
        wm_drv_pwm_channel_start(pwm_device, cfg.channel);
        wm_drv_gpio_iomux_func_sel(LED_R_PIN, WM_GPIO_IOMUX_FUN1);

        cfg.channel = WM_PWM_CHANNEL_0;
        wm_drv_pwm_set_channel_duty(pwm_device, cfg.channel, cfg.duty_cycle);
        wm_drv_pwm_channel_init(pwm_device, &cfg);
        wm_drv_pwm_channel_start(pwm_device, cfg.channel);
        wm_drv_gpio_iomux_func_sel(LED_G_PIN, WM_GPIO_IOMUX_FUN1);

        cfg.channel = WM_PWM_CHANNEL_2;
        wm_drv_pwm_set_channel_duty(pwm_device, cfg.channel, cfg.duty_cycle);
        wm_drv_pwm_channel_init(pwm_device, &cfg);
        wm_drv_pwm_channel_start(pwm_device, cfg.channel);
        wm_drv_gpio_iomux_func_sel(LED_B_PIN, WM_GPIO_IOMUX_FUN1);
    }
#endif
}

#ifdef __cplusplus
}
#endif
