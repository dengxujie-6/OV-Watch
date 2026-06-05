#include "LVGL_Task.h"

#include "cmsis_os2.h"
#include "debug_overlay.h"
#include "hwaccess.h"
#include "Key_task.h"
#include "lvgl.h"
#include "main.h"
#include "menu_page.h"
#include "page_manager.h"

#define LVGL_TASK_DELAY_MS 5U

static uint8_t lvgl_task_screen_on = 1U;

static void LVGL_Task_HandleKeyEvents(void);

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

    // 进入菜单首页，页面栈由 PageManager 负责维护。
    (void)PageManager_Push(&MenuPage);

    for(;;) {
        LVGL_Task_HandleKeyEvents();
        // 周期执行 LVGL 定时器处理函数，驱动动画、输入和刷新流程。
        (void)lv_timer_handler();
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
