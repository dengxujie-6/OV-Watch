/**
 * @file calendar_page.h
 * @brief 日历页面接口。
 *
 * 日历页面属于 Application/UI App 层，只负责创建 LVGL 日历 UI 和处理页面事件。
 */

#pragma once

#include "lvgl/lvgl.h"
#include "page_manager.h"

typedef struct calendar_page calendar_page_t;

extern const GUI_Page_t CalendarPage;

/**
 * @brief 创建日历页面对象。
 *
 * @return 成功返回页面对象指针，失败返回 NULL。返回对象由 calendar_page_destroy() 释放。
 */
calendar_page_t * calendar_page_create(void);

/**
 * @brief 销毁日历页面对象。
 *
 * @param page calendar_page_create() 返回的页面对象，可为 NULL。
 */
void calendar_page_destroy(calendar_page_t * page);

/**
 * @brief 获取日历页面根 screen 对象。
 *
 * @param page 日历页面对象，不能为 NULL。
 * @return 页面根 LVGL 对象；参数无效时返回 NULL。
 */
lv_obj_t * calendar_page_root(calendar_page_t * page);
