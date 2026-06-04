/**
 * @file stopwatch_page.c
 * @brief 秒表页面实现。
 *
 * 页面使用 LVGL tick 维护最小秒表状态，不在页面里直接操作硬件定时器。
 */

#include "stopwatch_page.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/**
 * @brief 秒表页面对象。
 *
 * root 是页面根 screen。minute_label、second_label、ms_label 分段固定宽度显示，
 * 避免数字宽度变化导致时间显示抖动。
 */
struct stopwatch_page {
    lv_obj_t * root;            /**< 页面根 screen，归本页面对象所有。 */
    lv_obj_t * minute_label;    /**< 分钟 label，root 删除时自动删除。 */
    lv_obj_t * second_label;    /**< 秒 label，root 删除时自动删除。 */
    lv_obj_t * ms_label;        /**< 两位毫秒 label，root 删除时自动删除。 */
    lv_obj_t * stop_icon;       /**< 停止按钮图标 label，root 删除时自动删除。 */
    lv_obj_t * toggle_icon;     /**< 开始/暂停按钮图标 label，root 删除时自动删除。 */
    lv_timer_t * refresh_timer; /**< 页面显示刷新定时器，由本页面删除。 */
    uint8_t last_minute;        /**< 上一次显示的分钟值，用于避免重复重绘。 */
    uint8_t last_second;        /**< 上一次显示的秒值，用于避免重复重绘。 */
    uint8_t last_millisecond;   /**< 上一次显示的两位毫秒值，用于避免重复重绘。 */
    bool last_running;          /**< 上一次显示的运行状态，用于避免重复更新按钮图标。 */
    bool display_ready;         /**< false 表示首次刷新，需要完整写入所有 label。 */
};

static stopwatch_page_t * s_stopwatch_page;
static uint32_t s_stopwatch_elapsed_ms;
static uint32_t s_stopwatch_last_tick;
static bool s_stopwatch_running;

static void stopwatch_tick_update(void)
{
    uint32_t now = lv_tick_get();

    if(s_stopwatch_running) {
        s_stopwatch_elapsed_ms += now - s_stopwatch_last_tick;
    }

    s_stopwatch_last_tick = now;
}

static void stopwatch_page_update(stopwatch_page_t * page)
{
    if(!page || !page->minute_label || !page->second_label || !page->ms_label) return;

    stopwatch_tick_update();

    uint32_t total_seconds = s_stopwatch_elapsed_ms / 1000U;
    uint8_t minute = (uint8_t)((total_seconds / 60U) % 100U);
    uint8_t second = (uint8_t)(total_seconds % 60U);
    uint8_t millisecond = (uint8_t)((s_stopwatch_elapsed_ms % 1000U) / 10U);

    char text[4];

    // label_set_text 会让对应对象失效并触发重绘；数值没变时跳过，可减轻 SPI 刷屏压力。
    if(!page->display_ready || (page->last_minute != minute)) {
        lv_snprintf(text, sizeof(text), "%02u", minute);
        lv_label_set_text(page->minute_label, text);
        page->last_minute = minute;
    }

    if(!page->display_ready || (page->last_second != second)) {
        lv_snprintf(text, sizeof(text), "%02u", second);
        lv_label_set_text(page->second_label, text);
        page->last_second = second;
    }

    if(!page->display_ready || (page->last_millisecond != millisecond)) {
        lv_snprintf(text, sizeof(text), "%02u", millisecond);
        lv_label_set_text(page->ms_label, text);
        page->last_millisecond = millisecond;
    }

    if(page->toggle_icon && (!page->display_ready || (page->last_running != s_stopwatch_running))) {
        lv_label_set_text(page->toggle_icon, s_stopwatch_running ? LV_SYMBOL_PAUSE : LV_SYMBOL_PLAY);
        page->last_running = s_stopwatch_running;
    }

    page->display_ready = true;
}

static void stopwatch_refresh_timer_cb(lv_timer_t * t)
{
    stopwatch_page_t * page = (stopwatch_page_t *)lv_timer_get_user_data(t);
    stopwatch_page_update(page);
}

static void stopwatch_page_key_cb(lv_event_t * e)
{
    if(lv_event_get_code(e) != LV_EVENT_KEY) return;
    if(lv_event_get_key(e) == LV_KEY_ESC) (void)PageManager_Pop();
}

static void stopwatch_toggle_btn_cb(lv_event_t * e)
{
    if(lv_event_get_code(e) != LV_EVENT_CLICKED) return;

    stopwatch_page_t * page = (stopwatch_page_t *)lv_event_get_user_data(e);
    stopwatch_tick_update();
    s_stopwatch_running = !s_stopwatch_running;
    s_stopwatch_last_tick = lv_tick_get();
    stopwatch_page_update(page);
}

static void stopwatch_stop_btn_cb(lv_event_t * e)
{
    if(lv_event_get_code(e) != LV_EVENT_CLICKED) return;

    stopwatch_page_t * page = (stopwatch_page_t *)lv_event_get_user_data(e);
    s_stopwatch_elapsed_ms = 0U;
    s_stopwatch_running = false;
    s_stopwatch_last_tick = lv_tick_get();
    stopwatch_page_update(page);
}

static lv_obj_t * stopwatch_button_create(lv_obj_t * parent, const char * text, lv_color_t bg,
                                          lv_event_cb_t cb, stopwatch_page_t * page)
{
    lv_obj_t * btn = lv_btn_create(parent);
    lv_obj_set_size(btn, 96, 48);
    lv_obj_set_style_radius(btn, 14, 0);
    lv_obj_set_style_bg_color(btn, bg, 0);
    lv_obj_set_style_bg_color(btn, lv_color_mix(bg, lv_color_white(), 35), LV_STATE_PRESSED);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, page);
    lv_obj_add_event_cb(btn, stopwatch_page_key_cb, LV_EVENT_KEY, NULL);

    lv_obj_t * label = lv_label_create(btn);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    lv_obj_set_style_text_font(label, LV_FONT_DEFAULT, 0);
    lv_obj_center(label);

    return label;
}

static lv_obj_t * stopwatch_time_label_create(lv_obj_t * parent, int32_t x)
{
    lv_obj_t * label = lv_label_create(parent);
    lv_obj_set_size(label, 56, 44);
    lv_label_set_long_mode(label, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    lv_obj_set_style_text_font(label, LV_FONT_DEFAULT, 0);
    lv_obj_align(label, LV_ALIGN_CENTER, x, 0);
    return label;
}

static void stopwatch_colon_create(lv_obj_t * parent, int32_t x)
{
    lv_obj_t * label = lv_label_create(parent);
    lv_label_set_text(label, ":");
    lv_obj_set_style_text_color(label, lv_color_hex(0x868e96), 0);
    lv_obj_set_style_text_font(label, LV_FONT_DEFAULT, 0);
    lv_obj_align(label, LV_ALIGN_CENTER, x, -2);
}

stopwatch_page_t * stopwatch_page_create(void)
{
    static stopwatch_page_t page_storage;
    stopwatch_page_t * page = &page_storage;
    memset(page, 0, sizeof(*page));
    s_stopwatch_last_tick = lv_tick_get();

    page->root = lv_obj_create(NULL);
    lv_obj_set_size(page->root, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(page->root, lv_color_hex(0x0b0f12), 0);
    lv_obj_set_style_border_width(page->root, 0, 0);
    lv_obj_set_style_pad_all(page->root, 14, 0);
    lv_obj_set_scrollbar_mode(page->root, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_event_cb(page->root, stopwatch_page_key_cb, LV_EVENT_KEY, NULL);

    lv_obj_t * panel = lv_obj_create(page->root);
    lv_obj_set_size(panel, lv_pct(100), 110);
    lv_obj_align(panel, LV_ALIGN_TOP_MID, 0, 20);
    lv_obj_set_style_bg_color(panel, lv_color_hex(0x12181f), 0);
    lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(panel, 1, 0);
    lv_obj_set_style_border_color(panel, lv_color_hex(0x26313b), 0);
    lv_obj_set_style_radius(panel, 14, 0);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);

    page->minute_label = stopwatch_time_label_create(panel, -66);
    stopwatch_colon_create(panel, -31);
    page->second_label = stopwatch_time_label_create(panel, 0);
    stopwatch_colon_create(panel, 31);
    page->ms_label = stopwatch_time_label_create(panel, 66);

    lv_obj_t * btn_row = lv_obj_create(page->root);
    lv_obj_set_size(btn_row, lv_pct(100), 60);
    lv_obj_align(btn_row, LV_ALIGN_BOTTOM_MID, 0, -22);
    lv_obj_set_style_bg_opa(btn_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(btn_row, 0, 0);
    lv_obj_set_style_pad_all(btn_row, 0, 0);
    lv_obj_clear_flag(btn_row, LV_OBJ_FLAG_SCROLLABLE);

    page->stop_icon = stopwatch_button_create(btn_row, LV_SYMBOL_STOP, lv_color_hex(0xff6b6b),
                                              stopwatch_stop_btn_cb, page);
    lv_obj_align(lv_obj_get_parent(page->stop_icon), LV_ALIGN_LEFT_MID, 0, 0);

    page->toggle_icon = stopwatch_button_create(btn_row, LV_SYMBOL_PLAY, lv_color_hex(0x2b3440),
                                                stopwatch_toggle_btn_cb, page);
    lv_obj_align(lv_obj_get_parent(page->toggle_icon), LV_ALIGN_RIGHT_MID, 0, 0);

    page->refresh_timer = lv_timer_create(stopwatch_refresh_timer_cb, 20, page);
    stopwatch_page_update(page);

    return page;
}

void stopwatch_page_destroy(stopwatch_page_t * page)
{
    if(!page) return;
    if(page->refresh_timer) {
        lv_timer_del(page->refresh_timer);
        page->refresh_timer = NULL;
    }
    if(page->root) lv_obj_del(page->root);
    memset(page, 0, sizeof(*page));
}

lv_obj_t * stopwatch_page_root(stopwatch_page_t * page)
{
    return page ? page->root : NULL;
}

/**
 * @brief 创建并加载秒表页面。
 */
static void StopwatchPage_Create(void)
{
    lv_obj_t * root;

    if(s_stopwatch_page != NULL) {
        stopwatch_page_destroy(s_stopwatch_page);
        s_stopwatch_page = NULL;
    }

    s_stopwatch_page = stopwatch_page_create();
    if(s_stopwatch_page == NULL) {
        return;
    }

    root = stopwatch_page_root(s_stopwatch_page);
    if(root == NULL) {
        stopwatch_page_destroy(s_stopwatch_page);
        s_stopwatch_page = NULL;
        return;
    }

    lv_screen_load(root);
}

/**
 * @brief 释放秒表页面。
 */
static void StopwatchPage_Destroy(void)
{
    stopwatch_page_destroy(s_stopwatch_page);
    s_stopwatch_page = NULL;
}

const GUI_Page_t StopwatchPage = {
    .create = StopwatchPage_Create,
    .destroy = StopwatchPage_Destroy,
};
