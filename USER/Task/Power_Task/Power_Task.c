#include "Power_Task.h"

#include "FreeRTOS.h"
#include "Sensor_Task.h"
#include "cmsis_os2.h"
#include "hwaccess.h"
#include "low_power.h"
#include "task.h"

//  软件定时器每 500ms 触发一次；空闲阶段和电源键长按计数都以这个拍点为基准。
#define POWER_TASK_TIMER_PERIOD_MS            500U
//  亮屏空闲 10s 后进入第一阶段，先把背光调暗。
#define POWER_TASK_DIM_TIMEOUT_TICKS          20U
//  调暗后继续空闲至 15s 时进入第二阶段，执行灭屏与背光关闭。
#define POWER_TASK_DISPLAY_OFF_TIMEOUT_TICKS  30U
//  灭屏后继续空闲至 20s 时进入第三阶段，执行 STOP 低功耗。
#define POWER_TASK_STOP_TIMEOUT_TICKS         40U
//  第一阶段背光亮度值，当前沿用项目已有 0~10 档位。
#define POWER_TASK_DIM_BRIGHTNESS             5U
//  用户活动恢复时使用的正常背光亮度。
#define POWER_TASK_NORMAL_BRIGHTNESS          10U
//  电源键长按 4s 触发关机，避免误触直接断电。
#define POWER_TASK_SHUTDOWN_HOLD_MS           4000U
//  长按关机计数同样按 500ms 拍点累计，因此这里换算成拍数而不是毫秒。
#define POWER_TASK_SHUTDOWN_HOLD_TICKS        (POWER_TASK_SHUTDOWN_HOLD_MS / POWER_TASK_TIMER_PERIOD_MS)
//  开机或唤醒后，至少先观察到 1 个释放拍点才允许重新进入长按关机判定。
#define POWER_TASK_SHUTDOWN_ARM_RELEASE_TICKS 1U
//  STOP 被 MPU 中断唤醒后，抬腕判定窗口暂定为 500ms。
#define POWER_TASK_RAISE_WINDOW_MS            500U

typedef enum {
    POWER_TASK_DISPLAY_ACTIVE = 0,
    POWER_TASK_DISPLAY_DIMMED,
    POWER_TASK_DISPLAY_OFF,
} Power_TaskDisplayState_t;

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
static uint32_t power_task_power_key_release_ticks;
static uint8_t power_task_power_key_shutdown_armed;
static uint8_t power_task_power_key_tracking_active;
static Power_TaskDisplayState_t power_task_display_state;

volatile uint32_t g_power_task_shutdown_armed;
volatile uint32_t g_power_task_shutdown_tracking_active;
volatile uint32_t g_power_task_shutdown_hold_ticks;
volatile uint32_t g_power_task_shutdown_release_ticks;
volatile uint32_t g_power_task_shutdown_raw_pressed;
volatile uint32_t g_power_task_shutdown_path_stage;

static void Power_Task_TimerCallback(void *argument);
static void Power_Task_ResetIdleState(void);
static void Power_Task_RestoreActiveDisplay(void);
static void Power_Task_ApplyDimState(void);
static void Power_Task_ApplyDisplayOffState(void);
static void Power_Task_HandleStopSequence(void);
static void Power_Task_DisarmShutdownUntilRelease(void);
static void Power_Task_UpdateDebugState(uint8_t raw_pressed);
static void Power_Task_TimerUpdateShutdownState(uint32_t *set_flags);

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
                                POWER_TASK_EVENT_LCD_DIM |
                                POWER_TASK_EVENT_DISPLAY_OFF |
                                POWER_TASK_EVENT_STOP |
                                POWER_TASK_EVENT_SHUTDOWN;

    (void)argument;

    if(power_task_timer_handle != NULL) {
        (void)osTimerStart(power_task_timer_handle, POWER_TASK_TIMER_PERIOD_MS);
    }

    for(;;) {
        uint32_t flags = osEventFlagsWait(power_task_event_handle,
                                          wait_flags,
                                          osFlagsWaitAny,
                                          osWaitForever);

        if((flags & osFlagsError) != 0UL) {
            continue;  // ! 阻塞等待只处理真实事件位，错误返回值不能参与状态机判断
        }

        if((flags & POWER_TASK_EVENT_ACTIVITY) != 0UL) {
            Power_Task_RestoreActiveDisplay();
            continue;
        }

        if((flags & POWER_TASK_EVENT_SHUTDOWN) != 0UL) {
            g_power_task_shutdown_path_stage = 2U;
            if(HwAccess.power.close != NULL) {
                g_power_task_shutdown_path_stage = 3U;
                HwAccess.power.close();
                g_power_task_shutdown_path_stage = 4U;
            }

            for(;;) {
                (void)osDelay(1000U);
            }
        }

        if((flags & POWER_TASK_EVENT_STOP) != 0UL) {
            Power_Task_HandleStopSequence();
            continue;
        }

        if((flags & POWER_TASK_EVENT_DISPLAY_OFF) != 0UL) {
            Power_Task_ApplyDisplayOffState();
        }

        if((flags & POWER_TASK_EVENT_LCD_DIM) != 0UL) {
            Power_Task_ApplyDimState();
        }
    }
}

/**
 * @brief 通知低功耗模块发生了新的用户活动。
 */
void Power_Task_NotifyActivity(void)
{
    if(power_task_event_handle != NULL) {
        (void)osEventFlagsSet(power_task_event_handle, POWER_TASK_EVENT_ACTIVITY);
    }
}

/**
 * @brief 通知 Power_Task 收到一次经过去抖确认的电源键按下边沿。
 */
void Power_Task_NotifyPowerKeyPressed(void)
{
    power_task_power_key_tracking_active = 1U;
}

/**
 * @brief 500 ms 周期回调，用于累积超时并投递阶段事件。
 *
 * @param argument 软件定时器参数，当前未使用。
 */
static void Power_Task_TimerCallback(void *argument)
{
    uint32_t set_flags = 0UL;

    (void)argument;

    taskENTER_CRITICAL();

    power_task_idle_ticks++;

    if(power_task_idle_ticks == POWER_TASK_DIM_TIMEOUT_TICKS) {
        set_flags |= POWER_TASK_EVENT_LCD_DIM;
    }

    if(power_task_idle_ticks == POWER_TASK_DISPLAY_OFF_TIMEOUT_TICKS) {
        set_flags |= POWER_TASK_EVENT_DISPLAY_OFF;
    }

    if(power_task_idle_ticks == POWER_TASK_STOP_TIMEOUT_TICKS) {
        set_flags |= POWER_TASK_EVENT_STOP;
    }

    Power_Task_TimerUpdateShutdownState(&set_flags);

    taskEXIT_CRITICAL();

    if((set_flags != 0UL) && (power_task_event_handle != NULL)) {
        (void)osEventFlagsSet(power_task_event_handle, set_flags);
    }
}

/**
 * @brief 清零超时计数，并回到亮屏阶段。
 */
static void Power_Task_ResetIdleState(void)
{
    power_task_idle_ticks = 0UL;
}

/**
 * @brief 在定时器拍点中更新长按关机计数，并在达到阈值时置位关机事件。
 *
 * @param set_flags 定时器回调本轮准备投递的事件位集合，不能为空。
 */
static void Power_Task_TimerUpdateShutdownState(uint32_t *set_flags)
{
    uint8_t power_pressed;

    if(set_flags == NULL) {
        return;
    }

    if(HwAccess.key.is_pressed == NULL) {
        power_task_power_key_hold_ticks = 0UL;
        power_task_power_key_release_ticks = 0UL;
        power_task_power_key_shutdown_armed = 0U;
        power_task_power_key_tracking_active = 0U;
        g_power_task_shutdown_path_stage = 10U;
        Power_Task_UpdateDebugState(0U);
        return;
    }

    g_power_task_shutdown_path_stage = 11U;
    power_pressed = HwAccess.key.is_pressed(HWACCESS_KEY_SCREEN);
    g_power_task_shutdown_raw_pressed = power_pressed;

    if(power_pressed == 0U) {
        g_power_task_shutdown_path_stage = 12U;
        power_task_power_key_hold_ticks = 0UL;
        if((power_task_power_key_tracking_active != 0U) &&
           (power_task_power_key_release_ticks < POWER_TASK_SHUTDOWN_ARM_RELEASE_TICKS)) {
            power_task_power_key_release_ticks++;
        }
        if((power_task_power_key_tracking_active != 0U) &&
           (power_task_power_key_release_ticks >= POWER_TASK_SHUTDOWN_ARM_RELEASE_TICKS)) {
            power_task_power_key_shutdown_armed = 1U;
        }
        Power_Task_UpdateDebugState(power_pressed);
        return;
    }

    power_task_power_key_release_ticks = 0UL;

    if((power_task_power_key_tracking_active == 0U) ||
       (power_task_power_key_shutdown_armed == 0U)) {
        g_power_task_shutdown_path_stage = 13U;
        power_task_power_key_hold_ticks = 0UL;
        Power_Task_UpdateDebugState(power_pressed);
        return;
    }

    if(power_task_power_key_hold_ticks < POWER_TASK_SHUTDOWN_HOLD_TICKS) {
        g_power_task_shutdown_path_stage = 14U;
        power_task_power_key_hold_ticks++;
    }

    Power_Task_UpdateDebugState(power_pressed);
    g_power_task_shutdown_path_stage = 15U;
    if(power_task_power_key_hold_ticks >= POWER_TASK_SHUTDOWN_HOLD_TICKS) {
        *set_flags |= POWER_TASK_EVENT_SHUTDOWN;
    }
}

/**
 * @brief 恢复到亮屏正常显示阶段。
 */
static void Power_Task_RestoreActiveDisplay(void)
{
    Power_Task_DisarmShutdownUntilRelease();
    Power_Task_ResetIdleState();

    if(HwAccess.lcd.display_on != NULL) {
        HwAccess.lcd.display_on();
    }

    if(HwAccess.lcd.set_backlight != NULL) {
        HwAccess.lcd.set_backlight(POWER_TASK_NORMAL_BRIGHTNESS);
    }

    LowPower_RequestWakeRefresh();
    power_task_display_state = POWER_TASK_DISPLAY_ACTIVE;
}

/**
 * @brief 应用调暗背光状态。
 */
static void Power_Task_ApplyDimState(void)
{
    if(power_task_display_state != POWER_TASK_DISPLAY_ACTIVE) {
        return;
    }

    if(HwAccess.lcd.set_backlight != NULL) {
        HwAccess.lcd.set_backlight(POWER_TASK_DIM_BRIGHTNESS);
    }

    power_task_display_state = POWER_TASK_DISPLAY_DIMMED;
}

/**
 * @brief 应用灭屏状态。
 */
static void Power_Task_ApplyDisplayOffState(void)
{
    if(power_task_display_state == POWER_TASK_DISPLAY_OFF) {
        return;
    }

    if(HwAccess.lcd.set_backlight != NULL) {
        HwAccess.lcd.set_backlight(0U);
    }

    if(HwAccess.lcd.display_off != NULL) {
        HwAccess.lcd.display_off();
    }

    power_task_display_state = POWER_TASK_DISPLAY_OFF;
}

/**
 * @brief 执行一次 STOP 进入、唤醒判定与恢复流程。
 */
static void Power_Task_HandleStopSequence(void)
{
    uint32_t wake_flags = LowPower_EnterStop();

    for(;;) {
        if((wake_flags & LOW_POWER_WAKE_SOURCE_KEY2) != 0UL) {
            Sensor_Task_ForceActiveMode();
            LowPower_ResumeAfterStop();
            Power_Task_RestoreActiveDisplay();
            break;
        }

        if(((wake_flags & LOW_POWER_WAKE_SOURCE_MPU) != 0UL) &&
           (Sensor_Task_EvaluateRaiseWake(POWER_TASK_RAISE_WINDOW_MS) != 0U)) {
            LowPower_ResumeAfterStop();
            Power_Task_RestoreActiveDisplay();
            break;
        }

        wake_flags = LowPower_ReenterStop();
    }
}

/**
 * @brief 在开机或按键唤醒后，要求先检测到一次按键释放再允许长按关机。
 */
static void Power_Task_DisarmShutdownUntilRelease(void)
{
    power_task_power_key_hold_ticks = 0UL;
    power_task_power_key_release_ticks = 0UL;
    power_task_power_key_shutdown_armed = 0U;
    power_task_power_key_tracking_active = 0U;
    Power_Task_UpdateDebugState(g_power_task_shutdown_raw_pressed != 0U ? 1U : 0U);
}

/**
 * @brief 同步长按关机调试状态，便于在 Keil Watch 中直接观察。
 */
static void Power_Task_UpdateDebugState(uint8_t raw_pressed)
{
    g_power_task_shutdown_armed = power_task_power_key_shutdown_armed;
    g_power_task_shutdown_tracking_active = power_task_power_key_tracking_active;
    g_power_task_shutdown_hold_ticks = power_task_power_key_hold_ticks;
    g_power_task_shutdown_release_ticks = power_task_power_key_release_ticks;
    g_power_task_shutdown_raw_pressed = raw_pressed;
}
