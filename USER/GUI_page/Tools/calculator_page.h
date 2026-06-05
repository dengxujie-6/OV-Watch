/**
 * @file calculator_page.h
 * @brief 计算器页面接口。
 *
 * 计算器页面属于 Application/UI App 层，负责创建计算器 UI 和处理本地计算逻辑。
 */

#pragma once

#include "lvgl/lvgl.h"
#include "page_manager.h"

typedef struct calculator_page calculator_page_t;

extern const GUI_Page_t CalculatorPage;

/**
 * @brief 创建计算器页面对象。
 *
 * @return 成功返回页面对象指针，失败返回 NULL。返回对象由 calculator_page_destroy() 释放。
 */
calculator_page_t * calculator_page_create(void);

/**
 * @brief 销毁计算器页面对象。
 *
 * @param page calculator_page_create() 返回的页面对象，可为 NULL。
 */
void calculator_page_destroy(calculator_page_t * page);

/**
 * @brief 获取计算器页面根 screen 对象。
 *
 * @param page 计算器页面对象，不能为 NULL。
 * @return 页面根 LVGL 对象；参数无效时返回 NULL。
 */
lv_obj_t * calculator_page_root(calculator_page_t * page);
