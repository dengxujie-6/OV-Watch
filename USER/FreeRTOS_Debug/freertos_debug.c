#include "freertos_debug.h"
#include "cmsis_os2.h"

#if (FREERTOS_DEBUG_ENABLE != 0U)
volatile TaskHandle_t FreeRTOS_DebugStackOverflowTask;
volatile char *FreeRTOS_DebugStackOverflowTaskName;
#endif

#if (FREERTOS_STACK_MONITOR_ENABLE != 0U)
volatile FreeRTOS_DebugTaskInfo_t g_freertos_debug_tasks[FREERTOS_DEBUG_MAX_TASKS];
volatile uint32_t g_freertos_debug_task_count;

const osThreadAttr_t FreeRTOS_DebugMonitorTask_attributes = {
    .name = "freertosDebug",
    .stack_size = (configMINIMAL_STACK_SIZE + (FREERTOS_DEBUG_MAX_TASKS * 16U)) * sizeof(StackType_t),
    .priority = (osPriority_t)osPriorityLow,
};

static void FreeRTOS_Debug_UpdateStackMonitor(void);

static void FreeRTOS_DebugMonitorTask(void *argument)
{
    (void)argument;

    for(;;) {
        FreeRTOS_Debug_UpdateStackMonitor();
        osDelay(FREERTOS_DEBUG_MONITOR_PERIOD_MS);
    }
}

/**
 * @brief 注册需要监控栈余量的 FreeRTOS 任务。
 * @param task_handle 任务句柄，通常来自 osThreadNew() 的返回值。
 * @param task_name 任务名称，只保存字符串指针，不拷贝字符串内容。
 * @retval 1 注册成功，0 参数无效、重复注册或监控槽已满。
 */
uint8_t FreeRTOS_Debug_RegisterTask(TaskHandle_t task_handle, const char *task_name)
{
    uint32_t index;

    if(task_handle == NULL) {
        return 0U;
    }

    for(index = 0U; index < g_freertos_debug_task_count; index++) {
        if(g_freertos_debug_tasks[index].handle == task_handle) {
            return 0U;
        }
    }

    if(g_freertos_debug_task_count >= FREERTOS_DEBUG_MAX_TASKS) {
        return 0U;
    }

    index = g_freertos_debug_task_count;
    g_freertos_debug_tasks[index].name = task_name;
    g_freertos_debug_tasks[index].handle = task_handle;
    g_freertos_debug_tasks[index].stack_min_free = 0U;
    g_freertos_debug_tasks[index].stack_min_free_bytes = 0U;
    g_freertos_debug_task_count++;

    return 1U;
}

/**
 * @brief 刷新已注册任务的历史最小栈余量。
 *
 * uxTaskGetStackHighWaterMark() 返回任务历史最小剩余栈空间，单位是
 * StackType_t。这里同时换算为字节，便于在调试器中直接观察。
 * 本函数应在普通任务上下文调用，不要在 ISR 中调用。
 */
static void FreeRTOS_Debug_UpdateStackMonitor(void)
{
    uint32_t index;

    for(index = 0U; index < g_freertos_debug_task_count; index++) {
        const UBaseType_t stack_free =
            uxTaskGetStackHighWaterMark(g_freertos_debug_tasks[index].handle);

        g_freertos_debug_tasks[index].stack_min_free = stack_free;
        g_freertos_debug_tasks[index].stack_min_free_bytes =
            (uint32_t)stack_free * (uint32_t)sizeof(StackType_t);
    }
}

/**
 * @brief 创建 FreeRTOS 栈监控任务。
 * @retval 监控任务句柄，创建失败时返回 NULL。
 */
osThreadId_t FreeRTOS_Debug_CreateMonitorTask(void)
{
    static osThreadId_t monitor_task_handle;

    if(monitor_task_handle == NULL) {
        monitor_task_handle =
            osThreadNew(FreeRTOS_DebugMonitorTask, NULL, &FreeRTOS_DebugMonitorTask_attributes);
    }

    return monitor_task_handle;
}
#endif

/**
 * @brief FreeRTOS 任务栈溢出 Hook。
 *
 * 当 Core/Inc/FreeRTOSConfig.h 中 configCHECK_FOR_STACK_OVERFLOW 为 1 或 2 时，
 * FreeRTOS 会在任务切换检查到栈异常后回调本函数。这里保存任务句柄和任务名，
 * 然后停机，方便用调试器查看是哪一个任务溢出。
 */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
#if (FREERTOS_DEBUG_ENABLE != 0U)
    FreeRTOS_DebugStackOverflowTask = xTask;
    FreeRTOS_DebugStackOverflowTaskName = pcTaskName;
#else
    (void)xTask;
    (void)pcTaskName;
#endif

    taskDISABLE_INTERRUPTS();
    for(;;) {
    }
}
