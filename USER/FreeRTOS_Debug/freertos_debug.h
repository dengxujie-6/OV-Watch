/**
 * @file freertos_debug.h
 * @brief FreeRTOS 调试辅助模块：监控任务栈剩余量，并记录栈溢出任务。
 *
 * 使用方法：
 * 1. 任务创建成功后，调用 FreeRTOS_Debug_RegisterTask() 注册任务，FREERTOS_DEBUG_MAX_TASKS控制最多8个。
 * 2. 注册完成后，调用 FreeRTOS_Debug_CreateMonitorTask() 创建栈监控任务。
 * 3. 调试器中查看 g_freertos_debug_tasks[] 和栈溢出记录变量。
 *
 * 注意：本模块接口会调用 FreeRTOS API，不要在 ISR 中调用。
 */

#ifndef FREERTOS_DEBUG_H
#define FREERTOS_DEBUG_H

#include <stdint.h>
#include "FreeRTOS.h"
#include "cmsis_os2.h"
#include "task.h"

#ifndef FREERTOS_DEBUG_ENABLE
#define FREERTOS_DEBUG_ENABLE 1U
#endif

#ifndef FREERTOS_STACK_MONITOR_ENABLE
#define FREERTOS_STACK_MONITOR_ENABLE FREERTOS_DEBUG_ENABLE
#endif

#ifndef FREERTOS_DEBUG_MONITOR_PERIOD_MS
#define FREERTOS_DEBUG_MONITOR_PERIOD_MS 1000U
#endif

#ifndef FREERTOS_DEBUG_TASKLIST_REPORT_ENABLE
#define FREERTOS_DEBUG_TASKLIST_REPORT_ENABLE 0U
#endif

#ifndef FREERTOS_DEBUG_MAX_TASKS
#define FREERTOS_DEBUG_MAX_TASKS 8U
#endif

typedef struct
{
    const char *name;
    TaskHandle_t handle;
    UBaseType_t stack_min_free;
    uint32_t stack_min_free_bytes;
} FreeRTOS_DebugTaskInfo_t;

#if (FREERTOS_DEBUG_ENABLE != 0U)
extern volatile TaskHandle_t FreeRTOS_DebugStackOverflowTask;
extern volatile char *FreeRTOS_DebugStackOverflowTaskName;
#endif

#if (FREERTOS_STACK_MONITOR_ENABLE != 0U)
extern volatile FreeRTOS_DebugTaskInfo_t g_freertos_debug_tasks[FREERTOS_DEBUG_MAX_TASKS];
extern volatile uint32_t g_freertos_debug_task_count;

uint8_t FreeRTOS_Debug_RegisterTask(TaskHandle_t task_handle, const char *task_name);
osThreadId_t FreeRTOS_Debug_CreateMonitorTask(void);
#else
#define FreeRTOS_Debug_RegisterTask(task_handle, task_name) ((uint8_t)0U)
#define FreeRTOS_Debug_CreateMonitorTask()                  ((osThreadId_t)NULL)
#endif

#endif /* FREERTOS_DEBUG_H */
