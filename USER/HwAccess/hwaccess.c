#include "hwaccess.h"

#include "st7789v.h"

/**
 * @brief 全局硬件访问对象。
 *
 * 这个表把应用层硬件访问接口绑定到具体的 ST7789V BSP 实现。
 */
obj_HwAccess HwAccess = {
    .lcd = {
        .init = st7789_Init,
        .deinit = st7789_DeInit,
        .set_backlight = st7789_SetBacklight,
    },
};
