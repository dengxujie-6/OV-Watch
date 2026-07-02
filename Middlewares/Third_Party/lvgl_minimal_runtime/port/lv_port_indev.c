/**
 * @file lv_port_indev.c
 * @brief LVGL 输入设备移植层。
 */

#include "lvgl.h"

#include "CST816T.h"
#include "lv_port_indev.h"
#include "stm32f4xx_hal.h"

static lv_indev_t * touch_indev;
static lv_point_t last_touch_point;
//  用于识别“新的按下边沿”，避免手指持续按住时反复产生 activity。
static uint8_t touch_last_pressed;
//  输入移植层只记录发生过一次新的触摸活动，由 GUI 任务在安全上下文中取走并转成低功耗 activity 事件。
static volatile uint8_t touch_activity_pending;

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

/**
 * @brief 取走一次新的触摸按下活动。
 *
 * @return 1 表示自上次读取后至少发生过一次新的触摸按下；0 表示没有新的触摸活动。
 */
uint8_t lv_port_indev_take_activity(void)
{
    uint8_t pending = touch_activity_pending;

    touch_activity_pending = 0U;
    return pending;
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
        if(touch_last_pressed == 0U) {
            touch_activity_pending = 1U;  //  只在新的按下边沿上报一次，避免长按期间反复重置低功耗计时
        }

        touch_last_pressed = 1U;
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

    touch_last_pressed = 0U;

    data->point = last_touch_point;
    data->state = LV_INDEV_STATE_RELEASED;

    g_lvgl_touch_release_count++;
    g_lvgl_touch_last_state = (uint32_t)LV_INDEV_STATE_RELEASED;
    g_lvgl_touch_last_x = last_touch_point.x;
    g_lvgl_touch_last_y = last_touch_point.y;
}
