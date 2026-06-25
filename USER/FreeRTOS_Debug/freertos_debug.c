#include "freertos_debug.h"

#include "hwaccess.h"

#define FREERTOS_IDLE_REPORT_PERIOD_MS   2000U
#define FREERTOS_IDLE_REPORT_BUFFER_SIZE 768U
#define FREERTOS_DEBUG_OK_TEXT           "ok\r\n"

static uint16_t FreeRTOS_Debug_BuildIdleReport(char *buffer, uint16_t size);

#if (FREERTOS_DEBUG_ENABLE != 0U)
volatile TaskHandle_t FreeRTOS_DebugStackOverflowTask;
volatile char *FreeRTOS_DebugStackOverflowTaskName;
#endif

#if (FREERTOS_STACK_MONITOR_ENABLE != 0U)
volatile FreeRTOS_DebugTaskInfo_t g_freertos_debug_tasks[FREERTOS_DEBUG_MAX_TASKS];
volatile uint32_t g_freertos_debug_task_count;

static void FreeRTOS_Debug_UpdateStackMonitor(void);
static void FreeRTOS_Debug_ReportTaskList(void);

/**
 * @brief 在普通任务上下文中轮询 FreeRTOS 监控逻辑。
 *
 * 该接口设计给低优先级打印任务调用：
 * 1. 刷新已注册任务的历史最小剩余栈空间；
 * 2. 按内部节流周期决定是否输出一次任务列表。
 *
 * 禁止在 ISR 中调用。
 */
void FreeRTOS_Debug_Poll(void)
{
    FreeRTOS_Debug_UpdateStackMonitor();
    FreeRTOS_Debug_ReportTaskList();
}

/**
 * @brief 注册需要监控栈余量的 FreeRTOS 任务。
 * @param task_handle 任务句柄，通常来自 `osThreadNew()` 的返回值。
 * @param task_name 任务名称，只保存字符串指针，不拷贝内容。
 * @retval 1 表示注册成功，0 表示参数无效、重复注册或监控表已满。
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
 * @brief 刷新已注册任务的历史最小剩余栈空间。
 *
 * `uxTaskGetStackHighWaterMark()` 返回任务历史最小剩余栈空间，单位是
 * `StackType_t`。这里同步换算成字节，便于在调试器中直接观察。
 * 本函数只允许在普通任务上下文调用，不能在 ISR 中调用。
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
 * @brief 在低优先级打印路径中周期输出 FreeRTOS 任务列表。
 *
 * 任务信息只走现有蓝牙发送接口，不额外创建新的调试输出任务，
 * 这样可以避免打印类逻辑抢占 GUI、按键和传感器任务。
 */
static void FreeRTOS_Debug_ReportTaskList(void)
{
#if (FREERTOS_DEBUG_TASKLIST_REPORT_ENABLE == 0U)
    return;
#else
#if (configUSE_TRACE_FACILITY == 1)
    static TickType_t last_report_tick;
    static char report_buffer[FREERTOS_IDLE_REPORT_BUFFER_SIZE];
    TickType_t now_tick;
    uint16_t report_len;

    now_tick = xTaskGetTickCount();
    if((now_tick - last_report_tick) < pdMS_TO_TICKS(FREERTOS_IDLE_REPORT_PERIOD_MS)) {
        return;
    }

    if((HwAccess.bluetooth.is_enabled == NULL) ||
       (HwAccess.bluetooth.is_enabled() == 0U) ||
       (HwAccess.bluetooth.send == NULL)) {
        return;
    }

    last_report_tick = now_tick;
    report_len = FreeRTOS_Debug_BuildIdleReport(report_buffer,
                                                (uint16_t)sizeof(report_buffer));
    if(report_len == 0U) {
        return;
    }

    (void)HwAccess.bluetooth.send((const uint8_t *)FREERTOS_DEBUG_OK_TEXT,
                                  (uint16_t)(sizeof(FREERTOS_DEBUG_OK_TEXT) - 1U),
                                  50U);
    (void)HwAccess.bluetooth.send((const uint8_t *)report_buffer,
                                  report_len,
                                  50U);
#endif
#endif
}
#endif

/**
 * @brief FreeRTOS 任务栈溢出 Hook。
 *
 * 当 `configCHECK_FOR_STACK_OVERFLOW` 为 1 或 2 时，FreeRTOS 会在任务切换时
 * 检查到栈异常后回调本函数。这里保存任务句柄和任务名，方便调试器定位。
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

/**
 * @brief FreeRTOS 空闲任务 Hook。
 *
 * 任务列表输出已经迁移到普通任务轮询路径，Idle Hook 保持空。
 */
void vApplicationIdleHook(void)
{
}

/**
 * @brief 构建 FreeRTOS 任务列表串口报告。
 * @param buffer 输出缓冲区，由调用者持有。
 * @param size 缓冲区字节数。
 * @return 实际要发送的字节数，不含字符串结尾零。
 */
static uint16_t FreeRTOS_Debug_BuildIdleReport(char *buffer, uint16_t size)
{
    uint16_t offset = 0U;

    if((buffer == NULL) || (size < 2U)) {
        return 0U;
    }

    buffer[0] = '\0';

    // vTaskList() 会把任务名、状态、优先级、栈高水位和任务编号写入 buffer。
    vTaskList(buffer);

    while((offset < (size - 1U)) && (buffer[offset] != '\0')) {
        offset++;
    }

    return offset;
}
