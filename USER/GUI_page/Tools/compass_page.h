#ifndef COMPASS_PAGE_H
#define COMPASS_PAGE_H

#include "lvgl/lvgl.h"
#include "page_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct compass_page compass_page_t;

extern const GUI_Page_t CompassPage;

/**
 * @brief 创建指南针页面对象。
 *
 * @return 成功返回页面对象指针，失败返回 NULL。返回对象由 compass_page_destroy() 释放。
 */
compass_page_t * compass_page_create(void);

/**
 * @brief 销毁指南针页面对象。
 *
 * @param page compass_page_create() 返回的页面对象，可以为 NULL。
 */
void compass_page_destroy(compass_page_t * page);

/**
 * @brief 获取指南针页面根 screen。
 *
 * @param page 指南针页面对象，不能为 NULL。
 * @return 页面根 LVGL 对象；参数无效时返回 NULL。
 */
lv_obj_t * compass_page_root(compass_page_t * page);

/**
 * @brief 创建 240x280 指南针背景表盘。
 *
 * @param parent 父对象，不能为 NULL。创建出的所有背景对象都挂在内部根容器下。
 */
void CompassDial_Create(lv_obj_t * parent);

/**
 * @brief 删除 CompassDial_Create() 创建的指南针背景表盘。
 */
void CompassDial_Destroy(void);

#ifdef __cplusplus
}
#endif

#endif /* COMPASS_PAGE_H */
