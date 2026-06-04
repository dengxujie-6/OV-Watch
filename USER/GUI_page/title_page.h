/**
 * @file title_page.h
 * @brief 通用标题页面接口。
 *
 * 该页面用于尚未实现专用功能的菜单项，只显示对应标题并支持返回。
 */

#pragma once

#include "lvgl/lvgl.h"
#include "page_manager.h"

typedef struct title_page title_page_t;

extern const GUI_Page_t TitlePage;

/**
 * @brief 设置通用标题页下一次创建时显示的标题。
 *
 * 该函数只保存字符串指针，调用方传入的字符串需要在页面创建期间保持有效。
 *
 * @param title 标题字符串，NULL 时显示空字符串。
 */
void title_page_set_title(const char * title);

/**
 * @brief 创建通用标题页面对象。
 *
 * @param title 页面标题字符串，函数只在创建 label 时读取该字符串。
 * @return 成功返回页面对象指针，失败返回 NULL。返回对象由 title_page_destroy() 释放。
 */
title_page_t * title_page_create(const char * title);

/**
 * @brief 销毁通用标题页面对象。
 *
 * @param page title_page_create() 返回的页面对象，可为 NULL。
 */
void title_page_destroy(title_page_t * page);

/**
 * @brief 获取通用标题页面根 screen 对象。
 *
 * @param page 通用标题页面对象，不能为 NULL。
 * @return 页面根 LVGL 对象；参数无效时返回 NULL。
 */
lv_obj_t * title_page_root(title_page_t * page);
