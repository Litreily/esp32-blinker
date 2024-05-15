/* RGB LED Strip */

#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/rmt.h"
#include "ws2813.h"

static const char *TAG = "LED_STRIP";

led_status_t led_status = LED_OFF;
led_effect_t led_effect = LED_EFFECT_ALWAYS_ON;
led_info_t led_info = {LED_COLOR_WHITE_WARM, 100};
bool flag_circle = 0;

#define RMT_TX_CHANNEL RMT_CHANNEL_0
#define LED_NUMBER  CONFIG_WS2813_STRIP_LED_NUMBER

#define CHASE_SPEED_MS (10)
#define BREATHE_INTERVAL_MS (20)

/**
 * @brief Simple helper function, converting HSV color space to RGB color space
 *
 * Wiki: https://en.wikipedia.org/wiki/HSL_and_HSV
 *
 */
static void led_strip_hsv2rgb(uint32_t h, uint32_t s, uint32_t v, uint32_t *r, uint32_t *g, uint32_t *b)
{
    h %= 360; // h -> [0,360]
    uint32_t rgb_max = v * 2.55f;
    uint32_t rgb_min = rgb_max * (100 - s) / 100.0f;

    uint32_t i = h / 60;
    uint32_t diff = h % 60;

    // RGB adjustment amount by hue
    uint32_t rgb_adj = (rgb_max - rgb_min) * diff / 60;

    switch (i) {
    case 0:
        *r = rgb_max;
        *g = rgb_min + rgb_adj;
        *b = rgb_min;
        break;
    case 1:
        *r = rgb_max - rgb_adj;
        *g = rgb_max;
        *b = rgb_min;
        break;
    case 2:
        *r = rgb_min;
        *g = rgb_max;
        *b = rgb_min + rgb_adj;
        break;
    case 3:
        *r = rgb_min;
        *g = rgb_max - rgb_adj;
        *b = rgb_max;
        break;
    case 4:
        *r = rgb_min + rgb_adj;
        *g = rgb_min;
        *b = rgb_max;
        break;
    default:
        *r = rgb_max;
        *g = rgb_min;
        *b = rgb_max - rgb_adj;
        break;
    }
}

void led_effect_rainbow(led_strip_t *strip)
{
    uint32_t red = 64;
    uint32_t green = 0;
    uint32_t blue = 0;
    uint16_t hue = 0;
    uint16_t start_rgb = 0;

    // Show simple rainbow chasing pattern
    ESP_LOGI(TAG, "LED Rainbow Chase Start");
    while(led_status == LED_ON) {
        for (int i = 0; i < 3; i++) {
            for (int j = i; j < LED_NUMBER; j += 3) {
                if (flag_circle && (j == 0 || j == 2 || j == 14 || j == 16))
                    continue;
                // Build RGB values
                hue = j * 360 / LED_NUMBER + start_rgb;
                led_strip_hsv2rgb(hue, 100, 100, &red, &green, &blue);
                // Write RGB values to strip driver
                ESP_ERROR_CHECK(strip->set_pixel(strip, j, red, green, blue));
            }
            // Flush RGB values to LEDs
            ESP_ERROR_CHECK(strip->refresh(strip, 100));
            vTaskDelay(pdMS_TO_TICKS(CHASE_SPEED_MS));
            strip->clear(strip, 50);
            vTaskDelay(pdMS_TO_TICKS(CHASE_SPEED_MS));
        }
        start_rgb += 60;
    }
}

const uint8_t gama[64] = { 
    0,   1,    2,   3,   4,   5,   6,   7,   8,   10,  
    12,  14,  16,  18,  20,  22,  24,  26,  29,   32,  
    35,  38,  41,  44,  47,  50,  53,  57,  61,   65,  
    69,  73,  77,  81,  85,  89,  94,  99,  104, 109, 
    114, 119, 124, 129, 134, 140, 146, 152, 158, 164, 
    170, 176, 182, 188, 195, 202, 209, 216, 223, 230, 
    237, 244, 251, 255 };

void led_effect_breathe(led_strip_t *strip)
{
    uint32_t red = 0;
    uint32_t green = 0;
    uint32_t blue = 0;
    uint8_t count = 0;
    uint8_t count_flag = 1; // 1: lighting, 0: darking
    uint16_t interval;

    uint8_t flag_red = 0, flag_green = 1, flag_blue = 1;
    switch (led_info.color) {
        case LED_COLOR_WHITE_WARM:
            flag_red = 1;
            flag_green = 1;
            flag_blue = 1;
            break;
        case LED_COLOR_WHITE_COLD:
            flag_red = 0;
            flag_green = 1;
            flag_blue = 1;
            break;
        case LED_COLOR_YELLOW:
            flag_red = 1;
            flag_green = 1;
            flag_blue = 0;
            break;
        case LED_COLOR_BLUE:
            flag_red = 0;
            flag_green = 0;
            flag_blue = 1;
            break;
        case LED_COLOR_GREEN:
            flag_red = 0;
            flag_green = 1;
            flag_blue = 0;
            break;
        case LED_COLOR_RED:
            flag_red = 1;
            flag_green = 0;
            flag_blue = 0;
            break;
        default:
            break;
    }

    if (led_info.brightness <= 0)
        return;
    interval = BREATHE_INTERVAL_MS * 100 / led_info.brightness;

    ESP_LOGI(TAG, "LED Breathing Start");
    while(led_status == LED_ON) {
        red = flag_red?gama[count]:0;
        green = flag_green?gama[count]:0;
        blue = flag_blue?gama[count]:0;

        for (int i = 0; i < LED_NUMBER; i++) {
            if (flag_circle && (i == 0 || i == 2 || i == 14 || i == 16))
                continue;
            ESP_ERROR_CHECK(strip->set_pixel(strip, i, red, green, blue));
        }
        ESP_ERROR_CHECK(strip->refresh(strip, 100));
        vTaskDelay(pdMS_TO_TICKS(interval));

        if(count_flag)
            count++;
        else
            count--;

        if (count >= (63 * led_info.brightness/100) || count == 0)
            count_flag = !count_flag;
    }
}

void led_effect_always_on(led_strip_t *strip, uint32_t color)
{
    uint32_t red = (color >> 16) & 0xFF;
    uint32_t green = (color >> 8) & 0xFF;;
    uint32_t blue = color & 0xFF;
    red = red * led_info.brightness / 100;
    green = green * led_info.brightness / 100;
    blue = blue * led_info.brightness / 100;

    ESP_LOGI(TAG, "LED always on with color: 0x%06x", color);
    for (int i = 0; i < LED_NUMBER; i++) {
        if (flag_circle && (i == 0 || i == 2 || i == 14 || i == 16))
            continue;
        ESP_ERROR_CHECK(strip->set_pixel(strip, i, red, green, blue));
    }
    ESP_ERROR_CHECK(strip->refresh(strip, 100));

    while(led_status == LED_ON)
        vTaskDelay(100 / portTICK_RATE_MS);
}

/* 
 * func: led_flash_ctl
 * @times: 
 *   if times < 0, flash will keep going until led_status not LED_ON
 *   else times > 0, flash will only flash sometimes
 */
static void led_flash_ctl(led_strip_t *strip, uint32_t color1, uint32_t color2, uint8_t rate_hz, int8_t times)
{
    uint32_t red[2], green[2], blue[2];
    uint8_t flag_id = 0;
    uint16_t delay_ms = 0;

    if (rate_hz ==0 || times == 0)
        return;

    delay_ms = 500 / rate_hz;

    red[0] = (color1 >> 16) & 0xFF;
    green[0] = (color1 >> 8) & 0xFF;;
    blue[0] = color1 & 0xFF;
    red[1] = (color2 >> 16) & 0xFF;
    green[1] = (color2 >> 8) & 0xFF;;
    blue[1] = color2 & 0xFF;
    
    while ((times < 0 && led_status == LED_ON) || (times > 0)) {
        for (int i = 0; i < LED_NUMBER; i++) {
            ESP_ERROR_CHECK(strip->set_pixel(strip, i, red[flag_id], green[flag_id], blue[flag_id]));
        }
        ESP_ERROR_CHECK(strip->refresh(strip, 100));
        flag_id = !flag_id;
        if (flag_id && (times > 0))
            --times;
        vTaskDelay(delay_ms / portTICK_RATE_MS);
    }
}

/* flash led between color1 and color2 */
void led_effect_flash(led_strip_t *strip, uint32_t color1, uint32_t color2)
{
    ESP_LOGI("led_effect_flash", "LED flash between color 0x%06x and 0x%06x", color1, color2);
    led_flash_ctl(strip, color1, color2, 2, -1);
}

void led_effect_test(led_strip_t *strip, uint32_t color1, uint32_t color2, uint8_t rate_hz, uint8_t times)
{
    ESP_LOGI("led_effect_test", "LED flash between color 0x%06x and 0x%06x", color1, color2);
    led_flash_ctl(strip, color1, color2, rate_hz, times);
}

led_strip_t * led_ws2813_init(void)
{
    rmt_config_t config = RMT_DEFAULT_CONFIG_TX(CONFIG_WS2813_RMT_TX_GPIO, RMT_TX_CHANNEL);
    // set counter clock to 40MHz
    config.clk_div = 2;

    ESP_ERROR_CHECK(rmt_config(&config));
    ESP_ERROR_CHECK(rmt_driver_install(config.channel, 0, 0));

    // install ws2812 driver
    led_strip_config_t strip_config = LED_STRIP_DEFAULT_CONFIG(LED_NUMBER, (led_strip_dev_t)config.channel);
    led_strip_t *strip = led_strip_new_rmt_ws2812(&strip_config);
    if (!strip) {
        ESP_LOGE(TAG, "install WS2812 driver failed");
    }

    // Clear LED strip (turn off all LEDs)
    ESP_ERROR_CHECK(strip->clear(strip, 100));
    ESP_LOGI(TAG, "INIT WS2812 driver");
    return strip;
}

void led_ws2813_resume(void)
{
    ESP_ERROR_CHECK(rmt_driver_uninstall(RMT_TX_CHANNEL));
    led_ws2813_init();
}

