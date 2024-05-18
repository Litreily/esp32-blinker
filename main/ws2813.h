#ifndef WS2813_H
#define WS2813_H

#include "led_strip.h"

#define C_BLACK     (0x000000)
#define C_WHITE     (0xFFFFFF)
#define C_RED       (0xFF0000)
#define C_GREEN     (0x00FF00)
#define C_BLUE      (0x0000FF)
#define C_YELLOW    (0xFFFF00)
#define C_CYAN      (0x00FFFF)
#define C_MAGENTA   (0xFF00FF)
#define C_PURPLE    (0x800080)
#define C_ORANGE    (0xFFA500)

typedef enum {
    LED_OFF,
    LED_ON,
    LED_IDLE
} led_status_t;

typedef enum {
    LED_EFFECT_BREATHE,
    LED_EFFECT_RAINBOW,
    LED_EFFECT_ALWAYS_ON,
    LED_EFFECT_ALWAYS_ON_WHITE,
    LED_EFFECT_FLASH,
    LED_EFFECT_CIRCLE,
    LED_EFFECT_TEST,
    LED_EFFECT_MAX
} led_effect_t;

typedef enum {
    LED_COLOR_WHITE_WARM,
    LED_COLOR_WHITE_COLD,
    LED_COLOR_YELLOW,
    LED_COLOR_BLUE,
    LED_COLOR_GREEN,
    LED_COLOR_RED
} led_color_t;

typedef struct {
    led_color_t color;
    uint8_t brightness;
} led_info_t;

extern led_status_t led_status;
extern led_effect_t led_effect;
extern led_info_t led_info;
extern bool led_refresh;
extern bool flag_circle;

led_strip_t * led_ws2813_init(void);
void led_ws2813_resume(void);
void led_effect_rainbow(led_strip_t *strip);
void led_effect_breathe(led_strip_t *strip);
void led_effect_always_on(led_strip_t *strip, uint32_t color);
void led_effect_flash(led_strip_t *strip, uint32_t color1, uint32_t color2);
void led_effect_test(led_strip_t *strip, uint32_t color1, uint32_t color2, uint8_t rate_hz, uint8_t times);

#endif // WS2813_H
