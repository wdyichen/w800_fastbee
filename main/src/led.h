#ifndef __LED_H__
#define __LED_H__

#ifdef __cplusplus
extern "C" {
#endif

void led_on(void);

void led_off(void);

int led_state(void);

void led_init(void);

void led_brightness(int brightness);
void led_red(int brightness);
void led_green(int brightness);
void led_blue(int brightness);
void led_breathing(int color);

#ifdef __cplusplus
}
#endif

#endif /* __LED_H__ */
