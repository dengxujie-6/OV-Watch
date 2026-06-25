#include "Power_Task.h"

#include "FreeRTOS.h"
#include "cmsis_os2.h"
#include "hwaccess.h"
#include "low_power.h"
#include "task.h"

#define POWER_TASK_TIMER_PERIOD_MS          500U
#define POWER_TASK_LCD_SAVE_TIMEOUT_TICKS   10U
#define POWER_TASK_SLEEP_TIMEOUT_TICKS      16U
#define POWER_TASK_LCD_SAVE_BRIGHTNESS      5U
#define POWER_TASK_NORMAL_BRIGHTNESS        10U
#define POWER_TASK_POLL_PERIOD_MS           100U
#define POWER_TASK_SHUTDOWN_HOLD_MS         4000U
#define POWER_TASK_SHUTDOWN_HOLD_TICKS      (POWER_TASK_SHUTDOWN_HOLD_MS / POWER_TASK_POLL_PERIOD_MS)
#define POWER_TASK_DEBUG_BYPASS_LOW_POWER   1U

static osEventFlagsId_t power_task_event_handle;
static const osEventFlagsAttr_t power_task_event_attributes = {
    .name = "powerTaskEvt",
};

static osTimerId_t power_task_timer_handle;
static const osTimerAttr_t power_task_timer_attributes = {
    .name = "powerTaskTmr",
};

static uint32_t power_task_idle_ticks;
static uint32_t power_task_power_key_hold_ticks;
static uint8_t power_task_lcd_save_armed;
static uint8_t power_task_sleep_armed;
static uint8_t power_task_lcd_dimmed;

static void Power_Task_TimerCallback(void *argument);
static void Power_Task_ResetIdleState(void);
static uint8_t Power_Task_CheckShutdownRequest(void);

/**
 * @brief 创建 Power_Task 依赖的事件组和周期软件定时器。
 */
void Power_Task_InitObjects(void)
{
    power_task_event_handle = osEventFlagsNew(&power_task_event_attributes);
    power_task_timer_handle = osTimerNew(Power_Task_TimerCallback,
                                         osTimerPeriodic,
                                         NULL,
                                         &power_task_timer_attributes);

}

/**
 * @brief 低功耗执行任务入口。
 *
 * @param argument FreeRTOS 任务参数，当前未使用。
 */
void Power_Task(void *argument)
{
    const uint32_t wait_flags = POWER_TASK_EVENT_ACTIVITY |
                                POWER_TASK_EVENT_LCD_SAVE |
                                POWER_TASK_EVENT_SLEEP;

    (void)argument;
    (void)wait_flags;

#if (POWER_TASK_DEBUG_BYPASS_LOW_POWER != 0U)
    (void)power_task_lcd_dimmed;
    (void)Power_Task_ResetIdleState;

    for(;;) {
        if(Power_Task_CheckShutdownRequest() != 0U) {
            if(HwAccess.power.close != NULL) {
                HwAccess.power.close();
            }

            for(;;) {
                (void)osDelay(1000U);
            }
        }

        (void)osDelay(POWER_TASK_POLL_PERIOD_MS);
    }
#else

    if(power_task_timer_handle != NULL) {
        (void)osTimerStart(power_task_timer_handle, POWER_TASK_TIMER_PERIOD_MS);
    }

    for(;;) {
        uint32_t flags = osEventFlagsWait(power_task_event_handle,
                                          wait_flags,
                                          osFlagsWaitAny,
                                          POWER_TASK_POLL_PERIOD_MS);

        if(Power_Task_CheckShutdownRequest() != 0U) {
            if(HwAccess.power.close != NULL) {
                HwAccess.power.close();
            }

            for(;;) {
                (void)osDelay(1000U);
            }
        }

        if((flags & POWER_TASK_EVENT_ACTIVITY) != 0UL) {
            uint8_t need_restore_backlight;

            taskENTER_CRITICAL();
            need_restore_backlight = power_task_lcd_dimmed;
            power_task_lcd_dimmed = 0U;
            Power_Task_ResetIdleState();
            taskEXIT_CRITICAL();

            if(need_restore_backlight != 0U) {
                HwAccess.lcd.set_backlight(POWER_TASK_NORMAL_BRIGHTNESS);
            }

            continue;
        }

        if((flags & POWER_TASK_EVENT_SLEEP) != 0UL) {
            (void)LowPower_EnterSleep();

            taskENTER_CRITICAL();
            power_task_lcd_dimmed = 0U;
            Power_Task_ResetIdleState();
            taskEXIT_CRITICAL();

            continue;
        }

        if((flags & POWER_TASK_EVENT_LCD_SAVE) != 0UL) {
            uint8_t need_dim_backlight = 0U;

            taskENTER_CRITICAL();
            if(power_task_lcd_dimmed == 0U) {
                power_task_lcd_dimmed = 1U;
                need_dim_backlight = 1U;
            }
            taskEXIT_CRITICAL();

            if(need_dim_backlight != 0U) {
                HwAccess.lcd.set_backlight(POWER_TASK_LCD_SAVE_BRIGHTNESS);
            }
        }
    }
#endif
}

/**
 * @brief 通知低功耗模块发生新的用户活动。
 */
void Power_Task_NotifyActivity(void)
{
    if(power_task_event_handle != NULL) {
        (void)osEventFlagsSet(power_task_event_handle, POWER_TASK_EVENT_ACTIVITY);
    }
}

/**
 * @brief 500 ms 周期回调，用于累计息屏超时并投递事件位。
 *
 * @param argument 软件定时器参数，当前未使用。
 */
static void Power_Task_TimerCallback(void *argument)
{
    uint32_t set_flags = 0UL;

    (void)argument;

#if (POWER_TASK_DEBUG_BYPASS_LOW_POWER != 0U)
    return;
#endif

    taskENTER_CRITICAL();

    power_task_idle_ticks++;

    if((power_task_lcd_save_armed == 0U) &&
       (power_task_idle_ticks >= POWER_TASK_LCD_SAVE_TIMEOUT_TICKS)) {
        power_task_lcd_save_armed = 1U;
        set_flags |= POWER_TASK_EVENT_LCD_SAVE;
    }

    if((power_task_sleep_armed == 0U) &&
       (power_task_idle_ticks >= POWER_TASK_SLEEP_TIMEOUT_TICKS)) {
        power_task_sleep_armed = 1U;
        set_flags |= POWER_TASK_EVENT_SLEEP;
    }

    taskEXIT_CRITICAL();

    if((set_flags != 0UL) && (power_task_event_handle != NULL)) {
        (void)osEventFlagsSet(power_task_event_handle, set_flags);
    }
}

/**
 * @brief 清零超时计数和阶段标志。
 */
static void Power_Task_ResetIdleState(void)
{
    power_task_idle_ticks = 0UL;
    power_task_lcd_save_armed = 0U;
    power_task_sleep_armed = 0U;
}

/**
 * @brief 轮询电源键长按状态，满 4 秒返回关机请求。
 *
 * 这里不走 UI 事件层，避免页面逻辑耦合到底层电源保持控制。
 *
 * @return 1 表示应执行关机；0 表示继续运行。
 */
static uint8_t Power_Task_CheckShutdownRequest(void)
{
    uint8_t power_pressed;

    if(HwAccess.key.is_pressed == NULL) {
        power_task_power_key_hold_ticks = 0UL;
        return 0U;
    }

    power_pressed = HwAccess.key.is_pressed(HWACCESS_KEY_SCREEN);
    if(power_pressed == 0U) {
        power_task_power_key_hold_ticks = 0UL;
        return 0U;
    }

    if(power_task_power_key_hold_ticks < POWER_TASK_SHUTDOWN_HOLD_TICKS) {
        power_task_power_key_hold_ticks++;
    }

    return (power_task_power_key_hold_ticks >= POWER_TASK_SHUTDOWN_HOLD_TICKS) ? 1U : 0U;
}
