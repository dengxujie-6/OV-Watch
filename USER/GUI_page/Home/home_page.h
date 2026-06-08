/**
 * @file home_page.h
 * @brief 默认主页页面接口。
 *
 * 默认主页属于 PageManager 栈底常驻页面，负责显示手表首页并进入主菜单。
 */

#pragma once

#include "lvgl/lvgl.h"
#include "page_manager.h"

typedef struct home_page home_page_t;

extern const GUI_Page_t HomePage;

/**
 * @brief 创建默认主页页面对象。
 *
 * @return 成功返回页面对象指针，失败返回 NULL。返回对象由 home_page_destroy() 释放。
 */
home_page_t * home_page_create(void);

/**
 * @brief 销毁默认主页页面对象。
 *
 * @param page home_page_create() 返回的页面对象，可为 NULL。
 */
void home_page_destroy(home_page_t * page);

/**
 * @brief 获取默认主页根 screen 对象。
 *
 * @param page 默认主页页面对象，不能为 NULL。
 * @return 页面根 LVGL 对象；参数无效时返回 NULL。
 */
lv_obj_t * home_page_root(home_page_t * page);
