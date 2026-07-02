#ifndef POWER_TASK_H
#define POWER_TASK_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

//  任意用户活动都会置位该事件，用于恢复亮屏并清空低功耗空闲计数。
#define POWER_TASK_EVENT_ACTIVITY      (1UL << 0)
//  空闲达到第一阶段阈值后置位，只负责把背光调暗，不进入深度低功耗。
#define POWER_TASK_EVENT_LCD_DIM       (1UL << 1)
//  空闲达到第二阶段阈值后置位，用于执行灭屏和背光关闭。
#define POWER_TASK_EVENT_DISPLAY_OFF   (1UL << 2)
//  空闲达到第三阶段阈值后置位，用于执行 STOP 进入与唤醒判定。
#define POWER_TASK_EVENT_STOP          (1UL << 3)
//  电源键长按计数达到关机阈值后置位，用于执行整机关机保持脚拉低。
#define POWER_TASK_EVENT_SHUTDOWN      (1UL << 4)

/**
 * @brief 创建低功耗任务依赖的事件组和软件定时器对象。
 *
 * 该函数应在调度器启动前调用一次，用于集中创建 Power_Task 的 RTOS 对象。
 */
void Power_Task_InitObjects(void);

/**
 * @brief 低功耗执行任务入口。
 *
 * 任务阻塞等待事件组，统一处理亮屏复位、LCD 省电和 Sleep 进入流程。
 *
 * @param argument FreeRTOS 任务参数，当前未使用。
 */
void Power_Task(void *argument);

/**
 * @brief 通知低功耗模块发生了新的用户活动。
 *
 * 该接口用于清零息屏计时，并在 LCD 已进入省电亮度时恢复正常背光。
 */
void Power_Task_NotifyActivity(void);

/**
 * @brief 通知 Power_Task 收到一次经过去抖确认的电源键按下边沿。
 *
 * 该接口用于把“允许长按关机”的起点收敛到 Key_Task 的稳定按下事件，
 * 避免开机阶段的高电平保持或 GPIO 毛刺被误判成关机长按。
 */
void Power_Task_NotifyPowerKeyPressed(void);

#ifdef __cplusplus
}
#endif

#endif /* POWER_TASK_H */
