/**
 * @file animation_page.h
 * @brief FPS 动画测试页面接口。
 *
 * 动画测试页面属于 Application/UI App 层，只创建 LVGL 对象和定时器，
 * 不直接访问底层硬件。
 */

#pragma once

#include "lvgl/lvgl.h"
#include "page_manager.h"

typedef struct animation_page animation_page_t;

extern const GUI_Page_t AnimationPage;

/**
 * @brief 创建 FPS 动画测试页面对象。
 *
 * @return 成功返回页面对象指针，失败返回 NULL。返回对象由 animation_page_destroy() 释放。
 */
animation_page_t * animation_page_create(void);

/**
 * @brief 销毁 FPS 动画测试页面对象。
 *
 * @param page animation_page_create() 返回的页面对象，可为 NULL。
 */
void animation_page_destroy(animation_page_t * page);

/**
 * @brief 获取 FPS 动画测试页面根 screen 对象。
 *
 * @param page 动画测试页面对象，不能为 NULL。
 * @return 页面根 LVGL 对象；参数无效时返回 NULL。
 */
lv_obj_t * animation_page_root(animation_page_t * page);
