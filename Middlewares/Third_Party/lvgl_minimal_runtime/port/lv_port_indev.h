#ifndef LV_PORT_INDEV_H
#define LV_PORT_INDEV_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化 LVGL 输入设备移植层。
 */
void lv_port_indev_init(void);

/**
 * @brief 取走一次触摸活动事件。
 *
 * @return 1 表示自上次读取后至少发生过一次新的触摸按下；0 表示没有新的触摸活动。
 */
uint8_t lv_port_indev_take_activity(void);

#ifdef __cplusplus
}
#endif

#endif /* LV_PORT_INDEV_H */
