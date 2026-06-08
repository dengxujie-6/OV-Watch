/**
 * @file my_icon_fonts.h
 * @brief Iconfont 字体对象和图标字符串定义。
 *
 * 本文件只集中定义 iconfont 的字体声明和 UTF-8 图标字符串，页面代码不要散落裸字节。
 */

#ifndef MY_ICON_FONTS_H
#define MY_ICON_FONTS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl/lvgl.h"

/**
 * @brief 自定义 iconfont 字体，当前由 my_icon_fonts.c 提供。
 */
extern const lv_font_t my_icon_fonts;

/**
 * @brief 水滴/湿度图标，Unicode U+E62B，UTF-8: EE 98 AB。
 */
#define MY_ICON_WATER_DROP "\xEE\x98\xAB"
#define MY_ICON_SHUIDI MY_ICON_WATER_DROP

/**
 * @brief 运动步数图标，Unicode U+E646，UTF-8: EE 99 86。
 */
#define MY_ICON_SHOE "\xEE\x99\x86"
#define MY_ICON_STEPS MY_ICON_SHOE

/**
 * @brief 测试/未知图标，Unicode U+E6DA，UTF-8: EE 9B 9A。
 */
#define MY_ICON_SUN "\xEE\x9B\x9A"
#define MY_ICON_TEST MY_ICON_SUN

#ifdef __cplusplus
}
#endif

#endif /* MY_ICON_FONTS_H */
