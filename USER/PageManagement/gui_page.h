/**
 * @file gui_page.h
 * @brief GUI 页面描述符定义。
 */

#ifndef GUI_PAGE_H
#define GUI_PAGE_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief GUI 页面描述符。
 *
 * PageManager 只保存该描述符指针，并在页面切换时调用 create() / destroy()。
 * 页面内部的 LVGL 对象、定时器和私有状态由页面自己管理。
 */
typedef struct {
    void (*create)(void);     /**< 创建并加载页面。 */
    void (*destroy)(void);    /**< 销毁页面内部资源，可为 NULL。 */
} GUI_Page_t;

#ifdef __cplusplus
}
#endif

#endif /* GUI_PAGE_H */
