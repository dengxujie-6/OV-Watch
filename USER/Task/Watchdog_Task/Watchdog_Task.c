#include "Watchdog_Task.h"

#include "cmsis_os2.h"
#include "hwaccess.h"
#include "LVGL_Task.h"

#define WATCHDOG_TASK_FEED_PERIOD_MS     500U
#define WATCHDOG_TASK_STARTUP_GRACE_MS   5000U
#define WATCHDOG_TASK_LVGL_TIMEOUT_MS    2000U

volatile uint32_t WatchdogTask_LastFeedTick;
volatile uint32_t WatchdogTask_LastLvglHeartbeatTick;
volatile uint32_t WatchdogTask_SkipFeedCount;
volatile uint32_t WatchdogTask_LvglTimeoutCount;
volatile uint8_t WatchdogTask_LvglHealthy;

/**
 * @brief 外部硬件看门狗喂狗任务入口。
 */
void Watchdog_Task(void *argument)
{
    uint32_t start_tick;
    uint32_t now_tick;
    uint32_t lvgl_heartbeat_tick;

    (void)argument;

    HwAccess.watchdog.init();
    HwAccess.watchdog.enable();
    start_tick = osKernelGetTickCount();

    for(;;) {
        now_tick = osKernelGetTickCount();
        lvgl_heartbeat_tick = LVGL_Task_GetHeartbeatTick();
        WatchdogTask_LastLvglHeartbeatTick = lvgl_heartbeat_tick;

        if((lvgl_heartbeat_tick != 0U) &&
           ((now_tick - lvgl_heartbeat_tick) <= WATCHDOG_TASK_LVGL_TIMEOUT_MS)) {
            WatchdogTask_LvglHealthy = 1U;
            HwAccess.watchdog.feed();
            WatchdogTask_LastFeedTick = now_tick;
        } else if((lvgl_heartbeat_tick == 0U) &&
                  ((now_tick - start_tick) <= WATCHDOG_TASK_STARTUP_GRACE_MS)) {
            // 启动宽限期内允许喂狗，避免 LVGL 初始化尚未完成时误复位。
            WatchdogTask_LvglHealthy = 1U;
            HwAccess.watchdog.feed();
            WatchdogTask_LastFeedTick = now_tick;
        } else {
            // 关键任务心跳超时后停止喂狗，让外部硬件看门狗完成复位。
            WatchdogTask_LvglHealthy = 0U;
            WatchdogTask_SkipFeedCount++;
            if(lvgl_heartbeat_tick != 0U) {
                WatchdogTask_LvglTimeoutCount++;
            }
        }

        osDelay(WATCHDOG_TASK_FEED_PERIOD_MS);
    }
}
