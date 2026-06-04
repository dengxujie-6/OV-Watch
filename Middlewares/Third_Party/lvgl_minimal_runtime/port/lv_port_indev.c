/**
 * @file lv_port_indev.c
 * @brief LVGL 输入设备移植层。
 */

#include "lvgl.h"

#include "CST816T.h"

static lv_indev_t * touch_indev;
static lv_point_t last_touch_point;

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

    (void)indev;

    if((CST816T_ReadTouch(&point) == 0) && (point.is_pressed != 0U)) {
        last_touch_point.x = (int32_t)point.x;
        last_touch_point.y = (int32_t)point.y;
        data->point = last_touch_point;
        data->state = LV_INDEV_STATE_PRESSED;
        return;
    }

    data->point = last_touch_point;
    data->state = LV_INDEV_STATE_RELEASED;
}
