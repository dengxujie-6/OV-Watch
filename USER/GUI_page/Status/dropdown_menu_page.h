/**
 * @file dropdown_menu_page.h
 * @brief 下滑快捷菜单页面接口。
 */

#pragma once

#include "lvgl/lvgl.h"
#include "page_manager.h"

typedef struct dropdown_menu_page dropdown_menu_page_t;

extern const GUI_Page_t DropdownMenuPage;

/**
 * @brief 创建下滑快捷菜单页面对象。
 *
 * @return 成功返回页面对象指针，失败返回 NULL。
 */
dropdown_menu_page_t * dropdown_menu_page_create(void);

/**
 * @brief 销毁下滑快捷菜单页面对象。
 *
 * @param page 页面对象，允许为 NULL。
 */
void dropdown_menu_page_destroy(dropdown_menu_page_t * page);

/**
 * @brief 获取下滑快捷菜单页面根 screen。
 *
 * @param page 页面对象，不能为空。
 * @return 页面根对象；参数无效时返回 NULL。
 */
lv_obj_t * dropdown_menu_page_root(dropdown_menu_page_t * page);
