/**
 * @file page_manager.c
 * @brief 页面栈管理器实现。
 */

#include "page_manager.h"
#include "stdio.h"

static const GUI_Page_t * page_stack[PAGE_MANAGER_STACK_DEPTH];
static int top = -1;

/**
 * @brief 进入一个新页面，并允许之后返回。
 *
 * PageManager 只负责页面描述符入栈和调用页面 create/destroy，
 * 不保存、不删除、不操作页面内部对象。
 *
 * @param page 页面描述符，不能为 NULL，且 create 不能为 NULL。
 * @return 0 表示成功，负数表示失败。
 */
int PageManager_Push(const GUI_Page_t * page)
{
    if(page == NULL) {
        return -1;
    }

    if(page->create == NULL) {
        return -2;
    }

    if(top >= (PAGE_MANAGER_STACK_DEPTH - 1)) {
        return -3;
    }

    if(top > 0) {
        const GUI_Page_t * current = page_stack[top];

        // 栈底默认主页是常驻页面，离开时只切换 screen，不销毁其 LVGL 对象树。
        if((current != NULL) && (current->destroy != NULL)) {
            current->destroy();
        }
    }

    top++;
    page_stack[top] = page;
    page->create();

    return 0;
}

/**
 * @brief 返回上一个页面。
 *
 * 当前已经在首页或页面栈为空时，直接返回失败码，不访问越界栈元素。
 *
 * @return 0 表示成功，负数表示当前已经在首页、栈为空或页面无效。
 */
int PageManager_Pop(void)
{
    const GUI_Page_t * current;
    const GUI_Page_t * previous;

    if(top <= 0) {
        return -1;
    }

    current = page_stack[top];
    if((current != NULL) && (current->destroy != NULL)) {
        current->destroy();
    }

    page_stack[top] = NULL;
    top--;

    previous = page_stack[top];
    if((previous == NULL) || (previous->create == NULL)) {
        return -2;
    }
    // 回到栈底默认主页时，create 只负责加载已保留的 screen。

    previous->create();
    return 0;
}
