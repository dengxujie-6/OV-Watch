/**
 * @file menu_page.h
 * @brief 主菜单页面接口。
 *
 * 主菜单页面属于 Application/UI App 层，负责显示应用入口并通过 PageManager 跳转。
 */

#pragma once

#include "lvgl/lvgl.h"
#include "page_manager.h"

typedef struct menu_page menu_page_t;

extern const GUI_Page_t MenuPage;

/**
 * @brief 创建主菜单页面对象。
 *
 * @return 成功返回页面对象指针，失败返回 NULL。返回对象由 menu_page_destroy() 释放。
 */
menu_page_t * menu_page_create(void);

/**
 * @brief 销毁主菜单页面对象。
 *
 * @param page menu_page_create() 返回的页面对象，可为 NULL。
 */
void menu_page_destroy(menu_page_t * page);

/**
 * @brief 获取主菜单页面根 screen 对象。
 *
 * @param page 主菜单页面对象，不能为 NULL。
 * @return 页面根 LVGL 对象；参数无效时返回 NULL。
 */
lv_obj_t * menu_page_root(menu_page_t * page);
