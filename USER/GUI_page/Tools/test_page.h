#ifndef TEST_PAGE_H
#define TEST_PAGE_H

#include "lvgl/lvgl.h"
#include "page_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct test_page test_page_t;

extern const GUI_Page_t TestPage;

/**
 * @brief 创建测试页面对象。
 *
 * @return 成功返回页面对象指针，失败返回 NULL。返回对象由 test_page_destroy() 释放。
 */
test_page_t * test_page_create(void);

/**
 * @brief 销毁测试页面对象。
 *
 * @param page test_page_create() 返回的页面对象，允许为 NULL。
 */
void test_page_destroy(test_page_t * page);

/**
 * @brief 获取测试页面根 screen。
 *
 * @param page 测试页面对象，不能为 NULL。
 * @return 页面根 LVGL 对象；参数无效时返回 NULL。
 */
lv_obj_t * test_page_root(test_page_t * page);

#ifdef __cplusplus
}
#endif

#endif /* TEST_PAGE_H */
