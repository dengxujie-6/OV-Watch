/**
 * @file charging_page.c
 * @brief 充电检测页面实现。
 *
 * 该文件属于 Application/UI App 层。页面通过 HwAccess 读取已经缓存好的电量，
 * 不直接访问 PA2、ADC、GPIO 或 HAL，因此后续迁移 STM32F411 时仍保持 UI 可移植。
 */

#include "charging_page.h"

#include "hwaccess.h"

#include <string.h>

extern const lv_font_t my_font_source_han_20;

#define CHARGING_PAGE_REFRESH_MS 1000U
#define CHARGING_RING_SIZE       198
#define CHARGING_RING_WIDTH      5
#define CHARGING_BATTERY_MIN_MV  2750U
#define CHARGING_BATTERY_MAX_MV  4200U

/**
 * @brief 充电检测页面对象。
 *
 * root 是页面根 screen，arc/label 都是 root 的子对象，root 删除时会由 LVGL 自动删除。
 * refresh_timer 由本页面创建和删除，用于低频刷新缓存电量显示。
 */
struct charging_page {
    lv_obj_t * root;             /**< 页面根 screen，归本页面对象所有。 */
    lv_obj_t * arc;              /**< 电量圆环，root 删除时自动删除。 */
    lv_obj_t * percent_label;    /**< 电量百分比 label，root 删除时自动删除。 */
    lv_obj_t * status_label;     /**< 中文充电状态 label，root 删除时自动删除。 */
    lv_obj_t * time_label;       /**< 时间 label，当前没有 RTC 服务时显示占位时间。 */
    lv_timer_t * refresh_timer;  /**< 页面刷新定时器，由本页面删除。 */
    uint8_t last_percent;        /**< 上一次显示的电量百分比，避免重复重绘。 */
    uint8_t display_ready;       /**< 0 表示首次刷新，需要完整写入所有控件。 */
};

static charging_page_t * s_charging_page;

static void charging_page_key_cb(lv_event_t * e)
{
    if(lv_event_get_code(e) != LV_EVENT_KEY) return;
    if(lv_event_get_key(e) == LV_KEY_ESC) (void)PageManager_Pop();
}

static uint8_t charging_page_read_percent(void)
{
    uint16_t voltage_mv;

    if(HwAccess.power.get_battery_voltage_mv == NULL) {
        return 0U;
    }

    voltage_mv = HwAccess.power.get_battery_voltage_mv();
    if(voltage_mv <= CHARGING_BATTERY_MIN_MV) {
        return 0U;
    }

    if(voltage_mv >= CHARGING_BATTERY_MAX_MV) {
        return 100U;
    }

    // 页面只做显示换算，GPIO/ADC 细节仍由 HwAccess/Power/BSP 分层封装。
    return (uint8_t)(((uint32_t)(voltage_mv - CHARGING_BATTERY_MIN_MV) * 100U) /
                     (CHARGING_BATTERY_MAX_MV - CHARGING_BATTERY_MIN_MV));
}

static void charging_page_update(charging_page_t * page)
{
    uint8_t percent;
    uint8_t is_charging = 0U;

    if((page == NULL) || (page->arc == NULL) || (page->percent_label == NULL) ||
       (page->status_label == NULL)) {
        return;
    }

    if(HwAccess.power.is_charging != NULL) {
        is_charging = HwAccess.power.is_charging();
    }

    percent = charging_page_read_percent();
    if(percent > 100U) {
        percent = 100U;
    }

    if((page->display_ready == 0U) || (page->last_percent != percent)) {
        lv_arc_set_value(page->arc, percent);
        lv_label_set_text_fmt(page->percent_label, "%u%%", percent);
        page->last_percent = percent;
        page->display_ready = 1U;
    }

    lv_label_set_text(page->status_label, (is_charging != 0U) ? "正在充电 " : "未充电 ");
}

static void charging_page_refresh_timer_cb(lv_timer_t * timer)
{
    charging_page_t * page = (charging_page_t *)lv_timer_get_user_data(timer);
    charging_page_update(page);
}

charging_page_t * charging_page_create(void)
{
    static charging_page_t page_storage;
    charging_page_t * page = &page_storage;
    lv_obj_t * charge_icon;

    memset(page, 0, sizeof(*page));

    page->root = lv_obj_create(NULL);
    if(page->root == NULL) {
        return NULL;
    }

    lv_obj_set_size(page->root, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(page->root, lv_color_black(), 0);
    lv_obj_set_style_border_width(page->root, 0, 0);
    lv_obj_set_style_pad_all(page->root, 0, 0);
    lv_obj_set_scrollbar_mode(page->root, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(page->root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(page->root, charging_page_key_cb, LV_EVENT_KEY, NULL);

    // arc 是 root 的子对象，只用于显示电量，不接受触摸拖动。
    page->arc = lv_arc_create(page->root);
    lv_obj_set_size(page->arc, CHARGING_RING_SIZE, CHARGING_RING_SIZE);
    lv_obj_center(page->arc);
    lv_arc_set_range(page->arc, 0, 100);
    lv_arc_set_bg_angles(page->arc, 0, 360);
    lv_arc_set_rotation(page->arc, 270);
    lv_obj_remove_style(page->arc, NULL, LV_PART_KNOB | LV_STATE_ANY);
    lv_obj_remove_flag(page->arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_width(page->arc, CHARGING_RING_WIDTH, LV_PART_MAIN);
    lv_obj_set_style_arc_width(page->arc, CHARGING_RING_WIDTH, LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(page->arc, true, LV_PART_MAIN);
    lv_obj_set_style_arc_rounded(page->arc, true, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(page->arc, lv_color_hex(0x063820), LV_PART_MAIN);
    lv_obj_set_style_arc_color(page->arc, lv_color_hex(0x2cff78), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(page->arc, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(page->arc, 0, 0);

    charge_icon = lv_label_create(page->root);
    lv_label_set_text(charge_icon, LV_SYMBOL_CHARGE);
    lv_obj_set_style_text_color(charge_icon, lv_color_hex(0x2cff78), 0);
    // 使用默认符号字体缩放显示，避免引入整套 montserrat_48 大字体。
    lv_obj_set_style_text_font(charge_icon, LV_FONT_DEFAULT, 0);
    lv_obj_set_style_transform_zoom(charge_icon, 878, 0);
    // 缩放中心放在 label 中心，保证图标视觉中心仍与圆环中心对齐。
    lv_obj_update_layout(charge_icon);
    lv_obj_set_style_transform_pivot_x(charge_icon, lv_obj_get_width(charge_icon) / 2, 0);
    lv_obj_set_style_transform_pivot_y(charge_icon, lv_obj_get_height(charge_icon) / 2, 0);
    lv_obj_align(charge_icon, LV_ALIGN_CENTER, 0, -54);

    page->percent_label = lv_label_create(page->root);
    lv_obj_set_size(page->percent_label, 128, 58);
    lv_label_set_text(page->percent_label, "--%");
    lv_label_set_long_mode(page->percent_label, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_color(page->percent_label, lv_color_white(), 0);
    // 使用自定义 24px 字体缩放到约 48px，减少 Flash 中的大字体资源。
    lv_obj_set_style_text_font(page->percent_label, &my_font_source_han_20, 0);
    lv_obj_set_style_text_align(page->percent_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_transform_zoom(page->percent_label, 512, 0);
    // 固定百分比 label 的缩放中心，避免 0%/100% 宽度变化导致视觉偏移。
    lv_obj_set_style_transform_pivot_x(page->percent_label, 64, 0);
    lv_obj_set_style_transform_pivot_y(page->percent_label, 29, 0);
    lv_obj_align(page->percent_label, LV_ALIGN_CENTER, 0, 2);

    page->status_label = lv_label_create(page->root);
    lv_label_set_text(page->status_label, "正在充电");
    lv_obj_set_style_text_color(page->status_label, lv_color_hex(0x2cff78), 0);
    lv_obj_set_style_text_font(page->status_label, &my_font_source_han_20, 0);
    lv_obj_align(page->status_label, LV_ALIGN_CENTER, 0, 54);

    page->time_label = lv_label_create(page->root);
    lv_label_set_text(page->time_label, "10:09");
    lv_obj_set_style_text_color(page->time_label, lv_color_hex(0x8b9097), 0);
    lv_obj_set_style_text_font(page->time_label, LV_FONT_DEFAULT, 0);
    lv_obj_align(page->time_label, LV_ALIGN_BOTTOM_MID, 0, -18);

    page->refresh_timer = lv_timer_create(charging_page_refresh_timer_cb,
                                          CHARGING_PAGE_REFRESH_MS,
                                          page);
    charging_page_update(page);

    return page;
}

void charging_page_destroy(charging_page_t * page)
{
    if(page == NULL) {
        return;
    }

    if(page->refresh_timer != NULL) {
        lv_timer_del(page->refresh_timer);
        page->refresh_timer = NULL;
    }

    if(page->root != NULL) {
        lv_obj_del(page->root);
    }

    memset(page, 0, sizeof(*page));
}

lv_obj_t * charging_page_root(charging_page_t * page)
{
    return (page != NULL) ? page->root : NULL;
}

/**
 * @brief 创建并加载充电检测页面。
 */
static void ChargingPage_Create(void)
{
    lv_obj_t * root;

    if(s_charging_page != NULL) {
        charging_page_destroy(s_charging_page);
        s_charging_page = NULL;
    }

    s_charging_page = charging_page_create();
    if(s_charging_page == NULL) {
        return;
    }

    root = charging_page_root(s_charging_page);
    if(root == NULL) {
        charging_page_destroy(s_charging_page);
        s_charging_page = NULL;
        return;
    }

    lv_screen_load(root);
}

/**
 * @brief 释放充电检测页面。
 */
static void ChargingPage_Destroy(void)
{
    charging_page_destroy(s_charging_page);
    s_charging_page = NULL;
}

const GUI_Page_t ChargingPage = {
    .create = ChargingPage_Create,
    .destroy = ChargingPage_Destroy,
};
