/**
 * @file debug_overlay.c
 * @brief GUI 调试统计数据源。
 */

#include "debug_overlay.h"

#include "lvgl.h"

#define DEBUG_OVERLAY_UPDATE_MS 1000U

typedef struct {
    lv_timer_t * timer;        /**< 周期计算 FPS 的 LVGL 定时器。*/
    uint32_t refr_cnt;         /**< 上一个统计周期内的刷新完成次数。*/
    uint32_t last_tick;        /**< 上一次生成统计结果的 LVGL tick。*/
    uint32_t fps;              /**< 最近一次计算得到的 FPS。*/
    uint8_t initialized;       /**< 防止重复注册 display 回调和定时器。*/
} DebugOverlay_t;

static DebugOverlay_t s_debug_overlay;

static void DebugOverlay_DisplayEventCb(lv_event_t * e);
static void DebugOverlay_TimerCb(lv_timer_t * timer);

/**
 * @brief 初始化 GUI 调试统计。
 *
 * 本模块只统计 FPS，不再创建 top layer 浮层；显示职责固定交给测试页面。
 */
void DebugOverlay_Init(void)
{
    lv_display_t * display;

    if(s_debug_overlay.initialized != 0U) {
        return;
    }

    display = lv_display_get_default();
    if(display == NULL) {
        return;
    }

    s_debug_overlay.last_tick = lv_tick_get();
    s_debug_overlay.timer = lv_timer_create(DebugOverlay_TimerCb,
                                            DEBUG_OVERLAY_UPDATE_MS,
                                            &s_debug_overlay);
    if(s_debug_overlay.timer == NULL) {
        return;
    }

    (void)lv_display_add_event_cb(display,
                                  DebugOverlay_DisplayEventCb,
                                  LV_EVENT_REFR_READY,
                                  &s_debug_overlay);
    s_debug_overlay.initialized = 1U;
}

/**
 * @brief 读取最近一次计算得到的 FPS。
 */
uint32_t DebugOverlay_GetFps(void)
{
    return s_debug_overlay.fps;
}

/**
 * @brief 读取 LVGL 内置内存池的总量和已用量。
 *
 * @param total_bytes 输出 LVGL 内存池总字节数，允许为 NULL。
 * @param used_bytes 输出 LVGL 内存池已用字节数，允许为 NULL。
 */
void DebugOverlay_GetLvMem(uint32_t * total_bytes, uint32_t * used_bytes)
{
    lv_mem_monitor_t monitor;
    size_t used_size;

    lv_mem_monitor(&monitor);
    if(monitor.total_size >= monitor.free_size) {
        used_size = monitor.total_size - monitor.free_size;
    } else {
        used_size = 0U;
    }

    if(total_bytes != NULL) {
        *total_bytes = (uint32_t)monitor.total_size;
    }

    if(used_bytes != NULL) {
        *used_bytes = (uint32_t)used_size;
    }
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
 * @brief 每秒计算一次 FPS。
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
}
