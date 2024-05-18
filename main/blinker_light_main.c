/* *****************************************************************
 *
 * Download latest Blinker library here:
 * https://github.com/blinker-iot/blinker-freertos/archive/master.zip
 * 
 * 
 * Blinker is a cross-hardware, cross-platform solution for the IoT. 
 * It provides APP, device and server support, 
 * and uses public cloud services for data transmission and storage.
 * It can be used in smart home, data monitoring and other fields 
 * to help users build Internet of Things projects better and faster.
 * 
 * Docs: https://doc.blinker.app/
 *       https://github.com/blinker-iot/blinker-doc/wiki
 * 
 * *****************************************************************
 * 
 * Blinker 库下载地址:
 * https://github.com/blinker-iot/blinker-freertos/archive/master.zip
 * 
 * Blinker 是一套跨硬件、跨平台的物联网解决方案，提供APP端、设备端、
 * 服务器端支持，使用公有云服务进行数据传输存储。可用于智能家居、
 * 数据监测等领域，可以帮助用户更好更快地搭建物联网项目。
 * 
 * 文档: https://doc.blinker.app/
 *       https://github.com/blinker-iot/blinker-doc/wiki
 * 
 * *****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include "esp_log.h"
#include "esp_system.h"
#include "driver/gpio.h"

#include "blinker_api.h"
#include "ws2813.h"

static const char *TAG = "my_blinker";

#define BUTTON_OPEN    "btn-open"
#define BUTTON_CLOSE   "btn-close"
#define NUM_OPEN       "num-open"

static int count = 0;

/* set led effect
 * @new_effect : new led effect
 * @force : force led on whatever led is on or off now
 */
static void set_led_effect(led_effect_t new_effect, bool force)
{
    if (new_effect == LED_EFFECT_CIRCLE)
        flag_circle = !flag_circle;
    else
        led_effect = new_effect;

    if (led_status == LED_ON) {
        led_status = LED_OFF;
        while (led_status != LED_IDLE) 
            vTaskDelay(100 / portTICK_RATE_MS);
        led_status = LED_ON;
    }
    else if(force)
        led_status = LED_ON;
}

static void led_task(void* pvParameter)
{
    led_strip_t *strip = (led_strip_t *)pvParameter;
    if(!strip)
        vTaskDelete(NULL);

    while (true) {
        if (led_status != LED_ON) {
            vTaskDelay(100 / portTICK_RATE_MS);
            continue;
        }

        switch (led_effect) {
            case LED_EFFECT_BREATHE:
                led_effect_breathe(strip);
                break;
            case LED_EFFECT_RAINBOW:
                led_effect_rainbow(strip);
                break;
            case LED_EFFECT_ALWAYS_ON:
                led_effect_always_on(strip, C_YELLOW);
                break;
            case LED_EFFECT_ALWAYS_ON_WHITE:
                led_effect_always_on(strip, C_WHITE);
                break;
            case LED_EFFECT_FLASH:
                led_effect_flash(strip, C_YELLOW, C_BLACK);
                break;
            case LED_EFFECT_TEST:
                led_effect_test(strip, C_WHITE, C_BLACK, 2, 3);
                break;
            default:
                break;
        }

        ESP_ERROR_CHECK(strip->clear(strip, 100));
        led_status = LED_IDLE;
    }
}

void button_open_callback(const blinker_widget_param_val_t *val)
{
    ESP_LOGI(TAG, "button state: %s", val->s);
    count++;

    cJSON *param = cJSON_CreateObject();
    blinker_widget_switch(param, val->s);
    blinker_widget_print(BUTTON_OPEN, param);
    cJSON_Delete(param);

    cJSON *num_param = cJSON_CreateObject();
    blinker_widget_color(num_param, "#FF00FF");
    blinker_widget_text(num_param, "按键测试");
    blinker_widget_unit(num_param, "次");
    blinker_widget_value_number(num_param, count);
    blinker_widget_print(NUM_OPEN, num_param);
    cJSON_Delete(num_param);

    set_led_effect(rand() % LED_EFFECT_MAX, true);
}

void button_close_callback(const blinker_widget_param_val_t *val)
{
    ESP_LOGI(TAG, "button state: %s", val->s);
    cJSON *param = cJSON_CreateObject();
    blinker_widget_switch(param, val->s);
    blinker_widget_print(BUTTON_CLOSE, param);
    cJSON_Delete(param);
    
    led_status = LED_OFF;
}

static void data_callback(const char *data)
{
    ESP_LOGI(TAG, "data: %s", data);
    cJSON *info = cJSON_Parse(data);
    if (info == NULL) {
        cJSON_Delete(info);
        return;
    }
    if (cJSON_HasObjectItem(info, BLINKER_CMD_BRIGHTNESS)) {
        led_info.brightness = cJSON_GetObjectItem(info, BLINKER_CMD_BRIGHTNESS)->valueint;
        led_refresh = true;
    }
    cJSON_Delete(info);
}

static void miot_callback(const blinker_va_param_cb_t *val)
{
    ESP_LOGI(TAG, "miot type: %d", val->type);
    cJSON *state_param = cJSON_CreateObject();
    switch (val->type)
    {
    case BLINKER_PARAM_POWER_STATE:
        if (!val->s) {
            break;
        }
        if (!strcmp(val->s, BLINKER_CMD_ON)) {
            set_led_effect(LED_EFFECT_ALWAYS_ON_WHITE, true);
            blinker_miot_power_state(state_param, "on");
        } else if (!strcmp(val->s, BLINKER_CMD_OFF)) {
            led_status = LED_OFF;
            blinker_miot_power_state(state_param, "off");
        } else {
            blinker_miot_power_state(state_param, val->s);
        }
        blinker_miot_print(state_param);
        break;

    default:
        break;
    }
    cJSON_Delete(state_param);
}

void app_main()
{
    blinker_init();
    led_strip_t *strip = led_ws2813_init();
    xTaskCreate(led_task, "led_task", 2048, strip, 2, NULL);

    blinker_widget_add(BUTTON_OPEN, BLINKER_BUTTON, button_open_callback);
    blinker_widget_add(BUTTON_CLOSE, BLINKER_BUTTON, button_close_callback);
    blinker_data_handler(data_callback);
    blinker_miot_handler_register(miot_callback);
}
