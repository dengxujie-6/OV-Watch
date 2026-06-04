/**
 * @file charging_page.h
 * @brief 充电检测页面接口。
 *
 * 该页面属于 UI App 层，只负责显示充电状态和电量。
 * 充电 GPIO 和电池 ADC 数据由 BSP/HWAccess 层通过 HwAccess 提供。
 */

#ifndef __CHARGING_PAGE_H
#define __CHARGING_PAGE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include "page_manager.h"

typedef struct charging_page charging_page_t;

/**
 * @brief 创建充电检测页面。
 *
 * @return 页面对象指针，创建失败时返回 NULL。
 */
charging_page_t * charging_page_create(void);

/**
 * @brief 销毁充电检测页面。
 *
 * @param page 页面对象指针，可以为 NULL。
 */
void charging_page_destroy(charging_page_t * page);

/**
 * @brief 获取充电检测页面根对象。
 *
 * @param page 页面对象指针，可以为 NULL。
 * @return LVGL screen 根对象，失败时返回 NULL。
 */
lv_obj_t * charging_page_root(charging_page_t * page);

extern const GUI_Page_t ChargingPage;

#ifdef __cplusplus
}
#endif

#endif /* __CHARGING_PAGE_H */
