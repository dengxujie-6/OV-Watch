#ifndef HEART_RATE_PAGE_H
#define HEART_RATE_PAGE_H

#include "lvgl/lvgl.h"
#include "page_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct heart_rate_page heart_rate_page_t;

extern const GUI_Page_t HeartRatePage;

/**
 * @brief 创建心率页面对象。
 *
 * @return 成功返回页面对象指针，失败返回 NULL。返回对象由 heart_rate_page_destroy() 释放。
 */
heart_rate_page_t * heart_rate_page_create(void);

/**
 * @brief 销毁心率页面对象。
 *
 * @param page heart_rate_page_create() 返回的页面对象，可为 NULL。
 */
void heart_rate_page_destroy(heart_rate_page_t * page);

/**
 * @brief 获取心率页面根 screen。
 *
 * @param page 心率页面对象，不能为 NULL。
 * @return 页面根 LVGL 对象；参数无效时返回 NULL。
 */
lv_obj_t * heart_rate_page_root(heart_rate_page_t * page);

#ifdef __cplusplus
}
#endif

#endif /* HEART_RATE_PAGE_H */
