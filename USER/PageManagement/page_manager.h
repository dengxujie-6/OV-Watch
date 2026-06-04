/**
 * @file page_manager.h
 * @brief 页面栈管理器接口。
 */

#ifndef PAGE_MANAGER_H
#define PAGE_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "gui_page.h"

#ifndef PAGE_MANAGER_STACK_DEPTH
#define PAGE_MANAGER_STACK_DEPTH 8
#endif

/**
 * @brief 进入一个新页面，并允许之后返回。
 *
 * @param page 页面描述符，不能为 NULL，且 create 不能为 NULL。
 * @return 0 表示成功，负数表示失败。
 */
int PageManager_Push(const GUI_Page_t * page);

/**
 * @brief 返回上一个页面。
 *
 * @return 0 表示成功，负数表示当前已经在首页、栈为空或页面无效。
 */
int PageManager_Pop(void);

#ifdef __cplusplus
}
#endif

#endif /* PAGE_MANAGER_H */
