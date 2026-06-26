#include "LVGL_Task.h"

#include "cmsis_os2.h"
#include "debug_overlay.h"
#include "home_page.h"
#include "Key_task.h"
#include "low_power.h"
#include "Power_Task.h"
#include "lvgl.h"
#include "main.h"
#include "page_manager.h"

#define LVGL_TASK_DELAY_MS 2U
#define LVGL_MEM_MONITOR_PERIOD_MS 200U

static volatile uint32_t lvgl_task_heartbeat_tick;

static void LVGL_Task_HandleKeyEvents(void);
static void LVGL_Task_HandleWakeRefresh(void);
static void LVGL_Task_UpdateMemMonitor(void);

/**
 * @brief LVGL 内存池监控快照，供调试器与打印任务查看。
 *
 * 这些变量只在 LVGL 任务上下文中更新，避免跨任务直接调用 LVGL 内存 API。
 */
volatile lv_mem_monitor_t g_lvgl_mem_monitor;
volatile uint32_t g_lvgl_mem_total_size;
volatile uint32_t g_lvgl_mem_free_size;
volatile uint32_t g_lvgl_mem_free_biggest_size;
volatile uint32_t g_lvgl_mem_used_pct;
volatile uint32_t g_lvgl_mem_frag_pct;
volatile uint32_t g_lvgl_mem_max_used;
volatile uint32_t g_lvgl_mem_last_update_ms;
volatile uint32_t g_lvgl_task_phase;

void lv_port_disp_init(void);
void lv_port_indev_init(void);

/**
 * @brief LVGL 任务入口。
 *
 * 本任务负责：
 * 1. 初始化 LVGL 核心、显示与输入移植层；
 * 2. 加载首页；
 * 3. 周期执行 `lv_timer_handler()` 与内存监控更新。
 *
 * `g_lvgl_task_phase` 用于记录当前运行阶段，方便卡死后判断停在哪一步。
 */
void LVGL_Task(void *argument)
{
    (void)argument;

    g_lvgl_task_phase = 1U;
    lv_init();

    g_lvgl_task_phase = 2U;
    lv_tick_set_cb(HAL_GetTick);

    g_lvgl_task_phase = 3U;
    lv_port_disp_init();
    lv_port_indev_init();
    DebugOverlay_Init();

    g_lvgl_task_phase = 4U;
    (void)PageManager_Push(&HomePage);
    LVGL_Task_UpdateMemMonitor();

    g_lvgl_task_phase = 5U;

    for(;;) {
        g_lvgl_task_phase = 10U;
        LVGL_Task_HandleWakeRefresh();

        g_lvgl_task_phase = 11U;
        LVGL_Task_HandleKeyEvents();

        g_lvgl_task_phase = 12U;
        (void)lv_timer_handler();

        g_lvgl_task_phase = 13U;
        LVGL_Task_UpdateMemMonitor();

        g_lvgl_task_phase = 14U;
        lvgl_task_heartbeat_tick = osKernelGetTickCount();

        g_lvgl_task_phase = 15U;
        osDelay(LVGL_TASK_DELAY_MS);
    }
}

/**
 * @brief 获取 LVGL 任务最近一次完成主循环的系统 tick。
 *
 * @return 最近一次心跳 tick；返回 0 表示任务尚未完成第一轮循环。
 */
uint32_t LVGL_Task_GetHeartbeatTick(void)
{
    return lvgl_task_heartbeat_tick;
}

/**
 * @brief 在 LVGL 任务上下文中处理按键事件。
 */
static void LVGL_Task_HandleKeyEvents(void)
{
    uint32_t key_events = Key_Task_FetchEvents();

    if(key_events != 0UL) {
        Power_Task_NotifyActivity();
    }

    if((key_events & KEY_TASK_EVENT_BACK) != 0UL) {
        (void)PageManager_Pop();
    }
}

/**
 * @brief 处理低功耗唤醒后的整屏刷新请求。
 */
static void LVGL_Task_HandleWakeRefresh(void)
{
    lv_obj_t * active_screen;

    if(LowPower_TakeWakeRefreshRequest() == 0U) {
        return;
    }

    active_screen = lv_screen_active();
    if(active_screen != NULL) {
        lv_obj_invalidate(active_screen);
    }

    lv_refr_now(NULL);
}

/**
 * @brief 周期刷新 LVGL 内存池监控信息。
 *
 * 只允许在 GUI/LVGL 任务中调用 `lv_mem_monitor()`。
 */
static void LVGL_Task_UpdateMemMonitor(void)
{
    const uint32_t now_ms = HAL_GetTick();
    lv_mem_monitor_t mon;

    if((g_lvgl_mem_last_update_ms != 0U) &&
       ((now_ms - g_lvgl_mem_last_update_ms) < LVGL_MEM_MONITOR_PERIOD_MS)) {
        return;
    }

    lv_mem_monitor(&mon);

    g_lvgl_mem_monitor = mon;
    g_lvgl_mem_total_size = (uint32_t)mon.total_size;
    g_lvgl_mem_free_size = (uint32_t)mon.free_size;
    g_lvgl_mem_free_biggest_size = (uint32_t)mon.free_biggest_size;
    g_lvgl_mem_used_pct = (uint32_t)mon.used_pct;
    g_lvgl_mem_frag_pct = (uint32_t)mon.frag_pct;
    g_lvgl_mem_max_used = (uint32_t)mon.max_used;
    g_lvgl_mem_last_update_ms = now_ms;
}
