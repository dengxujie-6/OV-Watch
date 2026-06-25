/**
 * @file heart_rate_page.c
 * @brief 心率原始值页面实现。
 *
 * 本页面只负责在 GUI 任务上下文中读取 HwAccess 缓存，并显示 EM7028 最近一次测得的
 * 原始 PPG 数值，不直接访问 BSP/HAL 或触发采样。
 */

#include "heart_rate_page.h"

#include <string.h>

#include "hwaccess.h"

extern const lv_font_t my_font_source_han_20;
extern const lv_font_t my_font_source_han_38;

#define HEART_RATE_PAGE_REFRESH_MS 200U

/**
 * @brief 心率页面对象。
 *
 * root 是页面根 screen；title_label、value_label、hint_label 都随 root 生命周期自动释放；
 * timer 负责周期刷新显示文本，需要在页面销毁前显式删除。
 */
struct heart_rate_page {
    lv_obj_t * root;          /**< 页面根 screen，归本页面对象所有。 */
    lv_obj_t * title_label;   /**< 标题 label，root 删除时自动删除。 */
    lv_obj_t * value_label;   /**< 原始值显示 label，root 删除时自动删除。 */
    lv_obj_t * hint_label;    /**< 状态提示 label，root 删除时自动删除。 */
    lv_timer_t * timer;       /**< 周期刷新定时器，由本页面删除。 */
};

static heart_rate_page_t * s_heart_rate_page;

static void heart_rate_page_key_cb(lv_event_t * e);
static void heart_rate_page_timer_cb(lv_timer_t * timer);
static void heart_rate_page_update(heart_rate_page_t * page);
static void heart_rate_page_start_monitor(void);
static void heart_rate_page_stop_monitor(void);

/**
 * @brief 处理 ESC 返回键。
 */
static void heart_rate_page_key_cb(lv_event_t * e)
{
    if(lv_event_get_code(e) != LV_EVENT_KEY) {
        return;
    }

    if(lv_event_get_key(e) == LV_KEY_ESC) {
        (void)PageManager_Pop();
    }
}

/**
 * @brief 定时刷新页面显示。
 */
static void heart_rate_page_timer_cb(lv_timer_t * timer)
{
    heart_rate_page_t * page = (heart_rate_page_t *)lv_timer_get_user_data(timer);

    heart_rate_page_update(page);
}

static void heart_rate_page_start_monitor(void)
{
    if(HwAccess.em7028.start != NULL) {
        (void)HwAccess.em7028.start();
    }
}

static void heart_rate_page_stop_monitor(void)
{
    if(HwAccess.em7028.stop != NULL) {
        (void)HwAccess.em7028.stop();
    }
}

/**
 * @brief 根据 EM7028 缓存状态刷新页面文本。
 *
 * 页面层只读取 HwAccess 暴露的缓存接口：
 * 1. `is_running()` 判断心率任务是否已启动采样；
 * 2. `is_valid()` 判断缓存里是否已经有过至少一次有效样本；
 * 3. `get_raw()` 读取最近一次 16 位原始 PPG 值。
 */
static void heart_rate_page_update(heart_rate_page_t * page)
{
    uint8_t is_running = 0U;
    uint8_t is_valid = 0U;
    uint16_t raw_value = 0U;

    if(page == NULL) {
        return;
    }

    if(HwAccess.em7028.is_running != NULL) {
        is_running = HwAccess.em7028.is_running();
    }

    if(HwAccess.em7028.is_valid != NULL) {
        is_valid = HwAccess.em7028.is_valid();
    }

    if((HwAccess.em7028.get_raw != NULL) && (is_valid != 0U)) {
        raw_value = HwAccess.em7028.get_raw();
    }

    if(page->value_label != NULL) {
        if(is_valid == 0U) {
            lv_label_set_text(page->value_label, "--");
        } else {
            lv_label_set_text_fmt(page->value_label, "%u", raw_value);
        }
    }

    if(page->hint_label == NULL) {
        return;
    }

    if(is_running == 0U) {
        lv_label_set_text(page->hint_label, "传感器未启动");
    } else if(is_valid == 0U) {
        lv_label_set_text(page->hint_label, "等待原始值...");
    } else {
        lv_label_set_text(page->hint_label, "EM7028 原始 PPG");
    }
}

heart_rate_page_t * heart_rate_page_create(void)
{
    static heart_rate_page_t page_storage;
    heart_rate_page_t * page = &page_storage;

    memset(page, 0, sizeof(*page));

    page->root = lv_obj_create(NULL);
    if(page->root == NULL) {
        return NULL;
    }

    lv_obj_set_size(page->root, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(page->root, lv_color_hex(0x0b0f12), 0);
    lv_obj_set_style_border_width(page->root, 0, 0);
    lv_obj_set_style_pad_all(page->root, 16, 0);
    lv_obj_set_scrollbar_mode(page->root, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(page->root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(page->root, heart_rate_page_key_cb, LV_EVENT_KEY, NULL);

    page->title_label = lv_label_create(page->root);
    if(page->title_label != NULL) {
        lv_label_set_text(page->title_label, "心率");
        lv_obj_set_style_text_font(page->title_label, &my_font_source_han_20, 0);
        lv_obj_set_style_text_color(page->title_label, lv_color_white(), 0);
        lv_obj_align(page->title_label, LV_ALIGN_TOP_LEFT, 2, 0);
    }

    page->value_label = lv_label_create(page->root);
    if(page->value_label != NULL) {
        lv_label_set_text(page->value_label, "--");
        lv_obj_set_style_text_font(page->value_label, &my_font_source_han_38, 0);
        lv_obj_set_style_text_color(page->value_label, lv_color_hex(0xff8787), 0);
        lv_obj_align(page->value_label, LV_ALIGN_CENTER, 0, -8);
    }

    page->hint_label = lv_label_create(page->root);
    if(page->hint_label != NULL) {
        lv_label_set_text(page->hint_label, "等待原始值...");
        lv_obj_set_style_text_font(page->hint_label, &my_font_source_han_20, 0);
        lv_obj_set_style_text_color(page->hint_label, lv_color_hex(0xadb5bd), 0);
        lv_obj_align(page->hint_label, LV_ALIGN_CENTER, 0, 42);
    }

    heart_rate_page_start_monitor();
    heart_rate_page_update(page);

    page->timer = lv_timer_create(heart_rate_page_timer_cb, HEART_RATE_PAGE_REFRESH_MS, page);
    return page;
}

void heart_rate_page_destroy(heart_rate_page_t * page)
{
    if(page == NULL) {
        return;
    }

    if(page->timer != NULL) {
        lv_timer_del(page->timer);
        page->timer = NULL;
    }

    heart_rate_page_stop_monitor();

    if(page->root != NULL) {
        lv_obj_del(page->root);
    }

    memset(page, 0, sizeof(*page));
}

lv_obj_t * heart_rate_page_root(heart_rate_page_t * page)
{
    return (page != NULL) ? page->root : NULL;
}

/**
 * @brief 创建并加载心率页面。
 */
static void HeartRatePage_Create(void)
{
    lv_obj_t * root;

    if(s_heart_rate_page != NULL) {
        heart_rate_page_destroy(s_heart_rate_page);
        s_heart_rate_page = NULL;
    }

    s_heart_rate_page = heart_rate_page_create();
    if(s_heart_rate_page == NULL) {
        return;
    }

    root = heart_rate_page_root(s_heart_rate_page);
    if(root == NULL) {
        heart_rate_page_destroy(s_heart_rate_page);
        s_heart_rate_page = NULL;
        return;
    }

    lv_screen_load(root);
}

/**
 * @brief 释放心率页面。
 */
static void HeartRatePage_Destroy(void)
{
    heart_rate_page_destroy(s_heart_rate_page);
    s_heart_rate_page = NULL;
}

const GUI_Page_t HeartRatePage = {
    .create = HeartRatePage_Create,
    .destroy = HeartRatePage_Destroy,
};
