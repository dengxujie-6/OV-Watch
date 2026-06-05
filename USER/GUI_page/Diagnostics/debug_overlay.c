#include "debug_overlay.h"

#include "hwaccess.h"
#include "lvgl.h"

#define DEBUG_OVERLAY_UPDATE_MS      1000U
#define DEBUG_OVERLAY_BATTERY_MIN_MV 2750U
#define DEBUG_OVERLAY_BATTERY_MAX_MV 4200U

typedef struct {
    lv_obj_t * label;          /**< 挂在 top layer 的调试文本对象。 */
    lv_timer_t * timer;        /**< 周期刷新 FPS 和电量的 LVGL 定时器。 */
    uint32_t refr_cnt;         /**< 上一个统计周期内的刷新完成次数。 */
    uint32_t last_tick;        /**< 上一次生成统计结果的 LVGL tick。 */
    uint32_t fps;              /**< 最近一次计算得到的 FPS。 */
} DebugOverlay_t;

static DebugOverlay_t s_debug_overlay;

static void DebugOverlay_DisplayEventCb(lv_event_t * e);
static void DebugOverlay_TimerCb(lv_timer_t * timer);
static uint8_t DebugOverlay_ReadBatteryPercent(uint8_t * valid);
static void DebugOverlay_RefreshLabel(void);

/**
 * @brief 初始化全局调试浮层。
 */
void DebugOverlay_Init(void)
{
    lv_display_t * display;
    lv_obj_t * top_layer;

    if(s_debug_overlay.label != NULL) {
        return;
    }

    display = lv_display_get_default();
    if(display == NULL) {
        return;
    }

    top_layer = lv_layer_top();
    if(top_layer == NULL) {
        return;
    }

    s_debug_overlay.label = lv_label_create(top_layer);
    if(s_debug_overlay.label == NULL) {
        return;
    }

    lv_label_set_text(s_debug_overlay.label, "FPS:-- BAT:--%");
    lv_obj_set_style_text_color(s_debug_overlay.label, lv_color_white(), 0);
    lv_obj_set_style_text_font(s_debug_overlay.label, LV_FONT_DEFAULT, 0);
    lv_obj_set_style_bg_color(s_debug_overlay.label, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(s_debug_overlay.label, LV_OPA_70, 0);
    lv_obj_set_style_pad_left(s_debug_overlay.label, 4, 0);
    lv_obj_set_style_pad_right(s_debug_overlay.label, 4, 0);
    lv_obj_set_style_pad_top(s_debug_overlay.label, 2, 0);
    lv_obj_set_style_pad_bottom(s_debug_overlay.label, 2, 0);
    lv_obj_align(s_debug_overlay.label, LV_ALIGN_TOP_RIGHT, -4, 4);

    s_debug_overlay.last_tick = lv_tick_get();
    s_debug_overlay.timer = lv_timer_create(DebugOverlay_TimerCb,
                                            DEBUG_OVERLAY_UPDATE_MS,
                                            &s_debug_overlay);
    (void)lv_display_add_event_cb(display,
                                  DebugOverlay_DisplayEventCb,
                                  LV_EVENT_REFR_READY,
                                  &s_debug_overlay);
    DebugOverlay_RefreshLabel();
}

/**
 * @brief 统计显示刷新完成事件。
 */
static void DebugOverlay_DisplayEventCb(lv_event_t * e)
{
    DebugOverlay_t * overlay = (DebugOverlay_t *)lv_event_get_user_data(e);

    if((overlay == NULL) || (lv_event_get_code(e) != LV_EVENT_REFR_READY)) {
        return;
    }

    overlay->refr_cnt++;
}

/**
 * @brief 每秒计算一次 FPS 并刷新电量显示。
 */
static void DebugOverlay_TimerCb(lv_timer_t * timer)
{
    DebugOverlay_t * overlay = (DebugOverlay_t *)lv_timer_get_user_data(timer);
    uint32_t elapsed_ms;

    if(overlay == NULL) {
        return;
    }

    elapsed_ms = lv_tick_elaps(overlay->last_tick);
    if(elapsed_ms != 0U) {
        overlay->fps = (overlay->refr_cnt * 1000U) / elapsed_ms;
    }

    overlay->refr_cnt = 0U;
    overlay->last_tick = lv_tick_get();
    DebugOverlay_RefreshLabel();
}

/**
 * @brief 读取电池电量百分比。
 *
 * @param valid 输出参数，非 NULL；1 表示成功读取，0 表示当前读数无效。
 * @return 电池电量百分比，范围 0~100。
 */
static uint8_t DebugOverlay_ReadBatteryPercent(uint8_t * valid)
{
    uint16_t voltage_mv;

    if(valid == NULL) {
        return 0U;
    }

    *valid = 0U;

    if(HwAccess.power.get_battery_voltage_mv == NULL) {
        return 0U;
    }

    voltage_mv = HwAccess.power.get_battery_voltage_mv();
    if(voltage_mv == 0U) {
        return 0U;
    }

    *valid = 1U;

    if(voltage_mv <= DEBUG_OVERLAY_BATTERY_MIN_MV) {
        return 0U;
    }

    if(voltage_mv >= DEBUG_OVERLAY_BATTERY_MAX_MV) {
        return 100U;
    }

    // 调试显示使用线性估算，真实采样和分压换算仍由 BSP Power 模块完成。
    return (uint8_t)(((uint32_t)(voltage_mv - DEBUG_OVERLAY_BATTERY_MIN_MV) * 100U) /
                     (DEBUG_OVERLAY_BATTERY_MAX_MV - DEBUG_OVERLAY_BATTERY_MIN_MV));
}

/**
 * @brief 刷新浮层文本。
 */
static void DebugOverlay_RefreshLabel(void)
{
    uint8_t battery_valid;
    uint8_t battery_percent;

    if(s_debug_overlay.label == NULL) {
        return;
    }

    battery_percent = DebugOverlay_ReadBatteryPercent(&battery_valid);
    if(battery_valid == 0U) {
        lv_label_set_text_fmt(s_debug_overlay.label,
                              "FPS:%lu BAT:--%%",
                              (unsigned long)s_debug_overlay.fps);
    } else {
        lv_label_set_text_fmt(s_debug_overlay.label,
                              "FPS:%lu BAT:%u%%",
                              (unsigned long)s_debug_overlay.fps,
                              battery_percent);
    }
}
