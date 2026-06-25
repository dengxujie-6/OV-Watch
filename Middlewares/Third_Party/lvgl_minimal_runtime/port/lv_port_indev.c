/**
 * @file lv_port_indev.c
 * @brief LVGL 输入设备移植层。
 */

#include "lvgl.h"

#include "CST816T.h"
#include "stm32f4xx_hal.h"

static lv_indev_t * touch_indev;
static lv_point_t last_touch_point;

/**
 * @brief 触摸输入调试变量。
 *
 * 这些变量用于 Keil Watch 观察触摸链路是否仍在更新，
 * 不参与正式功能逻辑。
 */
volatile uint32_t g_lvgl_touch_read_count;
volatile uint32_t g_lvgl_touch_press_count;
volatile uint32_t g_lvgl_touch_release_count;
volatile uint32_t g_lvgl_touch_error_count;
volatile uint32_t g_lvgl_touch_last_read_tick_ms;
volatile uint32_t g_lvgl_touch_last_state;
volatile int32_t g_lvgl_touch_last_x;
volatile int32_t g_lvgl_touch_last_y;
volatile int32_t g_lvgl_touch_last_error;

static void touch_read_cb(lv_indev_t * indev, lv_indev_data_t * data);

/**
 * @brief 初始化 LVGL 触摸输入设备。
 *
 * 触摸芯片驱动位于 BSP 层，本函数只把触摸数据适配成 LVGL pointer 输入。
 */
void lv_port_indev_init(void)
{
    touch_indev = lv_indev_create();
    if(touch_indev == NULL) {
        return;
    }

    lv_indev_set_type(touch_indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_display(touch_indev, lv_display_get_default());
    lv_indev_set_read_cb(touch_indev, touch_read_cb);
}

static void touch_read_cb(lv_indev_t * indev, lv_indev_data_t * data)
{
    CST816T_TouchPoint_t point;
    int touch_result;

    (void)indev;

    g_lvgl_touch_read_count++;
    g_lvgl_touch_last_read_tick_ms = HAL_GetTick();

    touch_result = CST816T_ReadTouch(&point);
    g_lvgl_touch_last_error = touch_result;

    if((touch_result == 0) && (point.is_pressed != 0U)) {
        last_touch_point.x = (int32_t)point.x;
        last_touch_point.y = (int32_t)point.y;
        data->point = last_touch_point;
        data->state = LV_INDEV_STATE_PRESSED;

        g_lvgl_touch_press_count++;
        g_lvgl_touch_last_state = (uint32_t)LV_INDEV_STATE_PRESSED;
        g_lvgl_touch_last_x = last_touch_point.x;
        g_lvgl_touch_last_y = last_touch_point.y;
        return;
    }

    if(touch_result != 0) {
        g_lvgl_touch_error_count++;
    }

    data->point = last_touch_point;
    data->state = LV_INDEV_STATE_RELEASED;

    g_lvgl_touch_release_count++;
    g_lvgl_touch_last_state = (uint32_t)LV_INDEV_STATE_RELEASED;
    g_lvgl_touch_last_x = last_touch_point.x;
    g_lvgl_touch_last_y = last_touch_point.y;
}
