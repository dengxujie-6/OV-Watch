#include "HardWare_Init_Task.h"

#include "cmsis_os2.h"
#include "hwaccess.h"

volatile uint32_t HardwareInit_DebugStage;

/**
 * @brief Initialize board hardware.
 *
 * 该任务只负责硬件启动顺序，任务创建由 freertos.c 统一完成。
 */
void HardWare_Init_Task(void *argument)
{
    (void)argument;

    HwAccess.watchdog.init();

    HardwareInit_DebugStage = 1U;
    HwAccess.power.open();
    HardwareInit_DebugStage = 2U;
    HwAccess.lcd.init();
    HardwareInit_DebugStage = 3U;
    HardwareInit_DebugStage = 4U;

    osThreadExit();
}
