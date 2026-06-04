#ifndef KEY_TASK_H
#define KEY_TASK_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define KEY_TASK_EVENT_BACK          (1UL << 0)
#define KEY_TASK_EVENT_SCREEN        (1UL << 1)

/**
 * @brief 按键扫描任务入口函数。
 *
 * 该任务把逻辑按键转换为 KEY_TASK_EVENT_xxx 事件，具体 GPIO 连接由 HwAccess 层封装。
 *
 * @param argument FreeRTOS 任务参数，当前未使用。
 */
void Key_Task(void *argument);

/**
 * @brief 取出按键任务产生的事件。
 *
 * 调用后会清空已取出的事件位，建议由 LVGL_Task 周期调用。
 *
 * @return KEY_TASK_EVENT_xxx 事件位组合。
 */
uint32_t Key_Task_FetchEvents(void);

#ifdef __cplusplus
}
#endif

#endif /* KEY_TASK_H */
