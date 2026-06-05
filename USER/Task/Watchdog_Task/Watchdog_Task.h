#ifndef WATCHDOG_TASK_H
#define WATCHDOG_TASK_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 外部硬件看门狗喂狗任务入口。
 *
 * 任务启动后初始化并打开外部看门狗，然后按固定周期翻转 WDI。
 * @param argument CMSIS-OS2 任务参数，当前未使用。
 */
void Watchdog_Task(void *argument);

#ifdef __cplusplus
}
#endif

#endif /* WATCHDOG_TASK_H */
