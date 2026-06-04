/**
 * @file stopwatch_page.h
 * @brief 秒表页面接口。
 *
 * 秒表页面属于 Application/UI App 层，负责显示分、秒、两位毫秒，并提供暂停和停止按钮。
 */

#pragma once

#include "lvgl/lvgl.h"
#include "page_manager.h"

typedef struct stopwatch_page stopwatch_page_t;

extern const GUI_Page_t StopwatchPage;

/**
 * @brief 创建秒表页面对象。
 *
 * @return 成功返回页面对象指针，失败返回 NULL。返回对象由 stopwatch_page_destroy() 释放。
 */
stopwatch_page_t * stopwatch_page_create(void);

/**
 * @brief 销毁秒表页面对象。
 *
 * @param page stopwatch_page_create() 返回的页面对象，可为 NULL。
 */
void stopwatch_page_destroy(stopwatch_page_t * page);

/**
 * @brief 获取秒表页面根 screen 对象。
 *
 * @param page 秒表页面对象，不能为 NULL。
 * @return 页面根 LVGL 对象；参数无效时返回 NULL。
 */
lv_obj_t * stopwatch_page_root(stopwatch_page_t * page);
