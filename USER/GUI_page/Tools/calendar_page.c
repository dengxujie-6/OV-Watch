/**
 * @file calendar_page.c
 * @brief 日历页面实现。
 *
 * 本页面只负责 LVGL 日历控件和页面返回事件。当前使用默认日期，
 * 后续可通过应用时间服务接入 RTC 或网络时间。
 */

#include "calendar_page.h"

#include <string.h>

extern const lv_font_t my_font_source_han_24;

/**
 * @brief 日历页面对象。
 *
 * root 是当前页面的 LVGL screen，由本页面创建并在 destroy 时删除。
 * title_label、calendar 都是 root 的子对象，生命周期跟随 root。
 */
struct calendar_page {
    lv_obj_t * root;                       /**< 页面根 screen，归本页面对象所有。 */
    lv_obj_t * title_label;                /**< 标题 label，root 删除时自动删除。 */
    lv_obj_t * calendar;                   /**< LVGL 日历控件，root 删除时自动删除。 */
    lv_calendar_date_t highlighted[1];     /**< 高亮日期缓存，供 LVGL 日历控件引用。 */
};

static calendar_page_t * s_calendar_page;

static void calendar_page_key_cb(lv_event_t * e)
{
    if(lv_event_get_code(e) != LV_EVENT_KEY) return;
    if(lv_event_get_key(e) == LV_KEY_ESC) {
        lv_event_stop_bubbling(e);
        (void)PageManager_Pop();
    }
}

static void calendar_page_bind_key_cb(lv_obj_t * obj)
{
    if(!obj) return;

    lv_obj_add_flag(obj, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_add_event_cb(obj, calendar_page_key_cb, LV_EVENT_KEY, NULL);

    uint32_t child_count = lv_obj_get_child_count(obj);
    for(uint32_t i = 0; i < child_count; i++) {
        calendar_page_bind_key_cb(lv_obj_get_child(obj, i));
    }
}

static lv_calendar_date_t calendar_page_get_today(void)
{
    return (lv_calendar_date_t) {
        .year = 2026,
        .month = 5,
        .day = 25,
    };
}

calendar_page_t * calendar_page_create(void)
{
    static const char * day_names[] = { "S", "M", "T", "W", "T", "F", "S" };
    static calendar_page_t page;
    calendar_page_t * page_ptr = &page;
    memset(page_ptr, 0, sizeof(*page_ptr));

    lv_calendar_date_t today = calendar_page_get_today();
    page_ptr->highlighted[0] = today;

    page_ptr->root = lv_obj_create(NULL);
    lv_obj_set_size(page_ptr->root, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(page_ptr->root, lv_color_hex(0x0b0f12), 0);
    lv_obj_set_style_border_width(page_ptr->root, 0, 0);
    lv_obj_set_style_pad_all(page_ptr->root, 16, 0);
    lv_obj_set_scrollbar_mode(page_ptr->root, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_event_cb(page_ptr->root, calendar_page_key_cb, LV_EVENT_KEY, NULL);

    page_ptr->title_label = lv_label_create(page_ptr->root);
    lv_label_set_text(page_ptr->title_label, "日历");
    lv_obj_set_style_text_font(page_ptr->title_label, &my_font_source_han_24, 0);
    lv_obj_set_style_text_color(page_ptr->title_label, lv_color_white(), 0);
    lv_obj_align(page_ptr->title_label, LV_ALIGN_TOP_LEFT, 2, 0);

    page_ptr->calendar = lv_calendar_create(page_ptr->root);
    lv_obj_set_size(page_ptr->calendar, lv_pct(100), 250);
    lv_obj_align(page_ptr->calendar, LV_ALIGN_TOP_MID, 0, 38);
    lv_obj_set_style_bg_color(page_ptr->calendar, lv_color_hex(0x12181f), 0);
    lv_obj_set_style_bg_opa(page_ptr->calendar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(page_ptr->calendar, 1, 0);
    lv_obj_set_style_border_color(page_ptr->calendar, lv_color_hex(0x26313b), 0);
    lv_obj_set_style_radius(page_ptr->calendar, 10, 0);
    lv_obj_set_style_pad_all(page_ptr->calendar, 8, 0);
    lv_obj_set_style_text_color(page_ptr->calendar, lv_color_hex(0xe9ecef), 0);
    lv_obj_set_style_text_font(page_ptr->calendar, LV_FONT_DEFAULT, 0);

    lv_obj_t * header = lv_calendar_add_header_arrow(page_ptr->calendar);
    lv_obj_set_style_text_color(header, lv_color_white(), 0);
    lv_obj_set_style_text_font(header, LV_FONT_DEFAULT, 0);

    lv_obj_t * matrix = lv_calendar_get_btnmatrix(page_ptr->calendar);
    lv_obj_set_style_bg_opa(matrix, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(matrix, 0, 0);
    lv_obj_set_style_text_color(matrix, lv_color_hex(0xe9ecef), LV_PART_ITEMS);
    lv_obj_set_style_text_color(matrix, lv_color_hex(0x868e96), LV_PART_ITEMS | LV_STATE_DISABLED);
    lv_obj_set_style_bg_color(matrix, lv_color_hex(0x2b3440), LV_PART_ITEMS | LV_STATE_CHECKED);
    lv_obj_set_style_bg_opa(matrix, LV_OPA_COVER, LV_PART_ITEMS | LV_STATE_CHECKED);
    lv_obj_set_style_radius(matrix, LV_RADIUS_CIRCLE, LV_PART_ITEMS | LV_STATE_CHECKED);

    lv_calendar_set_day_names(page_ptr->calendar, day_names);
    lv_calendar_set_today_date(page_ptr->calendar, today.year, today.month, today.day);
    lv_calendar_set_month_shown(page_ptr->calendar, today.year, today.month);
    lv_calendar_set_highlighted_dates(page_ptr->calendar, page_ptr->highlighted, 1);
    calendar_page_bind_key_cb(page_ptr->calendar);

    return page_ptr;
}

void calendar_page_destroy(calendar_page_t * page)
{
    if(!page) return;
    if(page->root) lv_obj_del(page->root);
    memset(page, 0, sizeof(*page));
}

lv_obj_t * calendar_page_root(calendar_page_t * page)
{
    return page ? page->root : NULL;
}

/**
 * @brief 创建并加载日历页面。
 */
static void CalendarPage_Create(void)
{
    lv_obj_t * root;

    if(s_calendar_page != NULL) {
        calendar_page_destroy(s_calendar_page);
        s_calendar_page = NULL;
    }

    s_calendar_page = calendar_page_create();
    if(s_calendar_page == NULL) {
        return;
    }

    root = calendar_page_root(s_calendar_page);
    if(root == NULL) {
        calendar_page_destroy(s_calendar_page);
        s_calendar_page = NULL;
        return;
    }

    lv_screen_load(root);
}

/**
 * @brief 释放日历页面。
 */
static void CalendarPage_Destroy(void)
{
    calendar_page_destroy(s_calendar_page);
    s_calendar_page = NULL;
}

const GUI_Page_t CalendarPage = {
    .create = CalendarPage_Create,
    .destroy = CalendarPage_Destroy,
};
