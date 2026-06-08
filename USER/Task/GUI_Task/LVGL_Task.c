#include "LVGL_Task.h"

#include "cmsis_os2.h"
#include "debug_overlay.h"
#include "home_page.h"
#include "hwaccess.h"
#include "Key_task.h"
#include "lvgl.h"
#include "main.h"
#include "page_manager.h"

#define LVGL_TASK_DELAY_MS 2U
#define LVGL_MEM_MONITOR_PERIOD_MS 200U

static uint8_t lvgl_task_screen_on = 1U;

static void LVGL_Task_HandleKeyEvents(void);
static void LVGL_Task_UpdateMemMonitor(void);

/**
 * @brief LVGL 堆内存监控快照，供调试器 Watch 窗口查看。
 *
 * 这些变量只在 LVGL 任务上下文更新，避免跨任务或中断直接调用 LVGL 内存 API。
 * 程序停在 LV_ASSERT_MALLOC / E7FE 时，优先看 free_biggest_size 是否过小。
 */
volatile lv_mem_monitor_t g_lvgl_mem_monitor;
volatile uint32_t g_lvgl_mem_total_size;
volatile uint32_t g_lvgl_mem_free_size;
volatile uint32_t g_lvgl_mem_free_biggest_size;
volatile uint32_t g_lvgl_mem_used_pct;
volatile uint32_t g_lvgl_mem_frag_pct;
volatile uint32_t g_lvgl_mem_max_used;
volatile uint32_t g_lvgl_mem_last_update_ms;

void lv_port_disp_init(void);
void lv_port_indev_init(void);

/**
 * @brief Run LVGL initialization and timer handling loop.
 *
 * 该任务由 freertos.c 统一创建，本文件只负责 LVGL 初始化和周期调度。
 */
void LVGL_Task(void *argument)
{
    (void)argument;

    // 初始化 LVGL 核心库，必须先于显示和输入设备初始化。
    lv_init();

    // 使用 HAL_GetTick 作为 LVGL 系统节拍来源。
    lv_tick_set_cb(HAL_GetTick);

    // 初始化显示驱动和输入设备驱动。
    lv_port_disp_init();
    lv_port_indev_init();
    DebugOverlay_Init();

    // 进入默认主页；该页位于 PageManager 栈底，切换到其它页面时保持常驻。
    (void)PageManager_Push(&HomePage);
    LVGL_Task_UpdateMemMonitor();

    for(;;) {
        LVGL_Task_HandleKeyEvents();
        // 周期执行 LVGL 定时器处理函数，驱动动画、输入和刷新流程。
        (void)lv_timer_handler();
        LVGL_Task_UpdateMemMonitor();
        osDelay(LVGL_TASK_DELAY_MS);
    }
}

/**
 * @brief 在 LVGL 任务上下文中处理按键事件。
 */
static void LVGL_Task_HandleKeyEvents(void)
{
    uint32_t key_events = Key_Task_FetchEvents();

    if((key_events & KEY_TASK_EVENT_BACK) != 0UL) {
        (void)PageManager_Pop();
    }

    if((key_events & KEY_TASK_EVENT_SCREEN) != 0UL) {
        if(lvgl_task_screen_on != 0U) {
            HwAccess.lcd.set_backlight(0U);
            HwAccess.lcd.display_off();
            lvgl_task_screen_on = 0U;
        } else {
            HwAccess.lcd.display_on();
            HwAccess.lcd.set_backlight(100U);
            lvgl_task_screen_on = 1U;
        }
    }
}

/**
 * @brief 定期刷新 LVGL 堆内存监控信息。
 *
 * lv_mem_monitor() 会遍历 LVGL 内部堆块，只允许在 GUI/LVGL 任务中调用。
 * 展开的 uint32_t 变量用于在 Keil Watch 窗口里直接观察关键水位。
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
