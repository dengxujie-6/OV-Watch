/**
 * @file title_page.c
 * @brief 通用标题页面实现。
 *
 * 该页面用于暂未实现专用功能的菜单入口，只显示标题并处理 ESC 返回。
 */

#include "title_page.h"

#include <string.h>

extern const lv_font_t my_font_source_han_20;

/**
 * @brief 通用标题页面对象。
 *
 * root 是页面根 screen，title_label 是 root 的子对象，root 删除时自动删除。
 */
struct title_page {
    lv_obj_t * root;           /**< 页面根 screen，归本页面对象所有。 */
    lv_obj_t * title_label;    /**< 标题 label，root 删除时自动删除。 */
};

static const char * s_title_page_title = "";
static title_page_t * s_title_page;

static void title_page_key_cb(lv_event_t * e)
{
    if(lv_event_get_code(e) != LV_EVENT_KEY) return;

    uint32_t key = lv_event_get_key(e);
    if(key == LV_KEY_ESC) {
        (void)PageManager_Pop();
    }
}

void title_page_set_title(const char * title)
{
    s_title_page_title = (title != NULL) ? title : "";
}

title_page_t * title_page_create(const char * title)
{
    static title_page_t page;
    memset(&page, 0, sizeof(page));

    page.root = lv_obj_create(NULL);
    lv_obj_set_size(page.root, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(page.root, lv_color_hex(0x0b0f12), 0);
    lv_obj_set_style_border_width(page.root, 0, 0);
    lv_obj_set_style_pad_all(page.root, 16, 0);
    lv_obj_set_scrollbar_mode(page.root, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_event_cb(page.root, title_page_key_cb, LV_EVENT_KEY, NULL);

    page.title_label = lv_label_create(page.root);
    lv_label_set_text(page.title_label, title ? title : "");
    lv_obj_set_style_text_font(page.title_label, &my_font_source_han_20, 0);
    lv_obj_set_style_text_color(page.title_label, lv_color_white(), 0);
    lv_obj_center(page.title_label);

    return &page;
}

void title_page_destroy(title_page_t * page)
{
    if(!page) return;
    if(page->root) lv_obj_del(page->root);
    memset(page, 0, sizeof(*page));
}

lv_obj_t * title_page_root(title_page_t * page)
{
    return page ? page->root : NULL;
}

/**
 * @brief 创建并加载通用标题页。
 */
static void TitlePage_Create(void)
{
    lv_obj_t * root;

    if(s_title_page != NULL) {
        title_page_destroy(s_title_page);
        s_title_page = NULL;
    }

    s_title_page = title_page_create(s_title_page_title);
    if(s_title_page == NULL) {
        return;
    }

    root = title_page_root(s_title_page);
    if(root == NULL) {
        title_page_destroy(s_title_page);
        s_title_page = NULL;
        return;
    }

    lv_screen_load(root);
}

/**
 * @brief 释放通用标题页。
 */
static void TitlePage_Destroy(void)
{
    title_page_destroy(s_title_page);
    s_title_page = NULL;
}

const GUI_Page_t TitlePage = {
    .create = TitlePage_Create,
    .destroy = TitlePage_Destroy,
};
