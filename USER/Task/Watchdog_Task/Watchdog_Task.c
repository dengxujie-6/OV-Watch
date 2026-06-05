#include "Watchdog_Task.h"

#include "cmsis_os2.h"
#include "hwaccess.h"

#define WATCHDOG_TASK_FEED_PERIOD_MS     500U

/**
 * @brief 外部硬件看门狗喂狗任务入口。
 */
void Watchdog_Task(void *argument)
{
    (void)argument;

    HwAccess.watchdog.init();
    HwAccess.watchdog.enable();

    for(;;) {
        HwAccess.watchdog.feed();
        osDelay(WATCHDOG_TASK_FEED_PERIOD_MS);
    }
}
