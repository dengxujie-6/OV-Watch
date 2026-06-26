#include "Print_Task.h"

#include <string.h>
#include <stdint.h>
#include <stdio.h>

#include "cmsis_os2.h"
#include "freertos_debug.h"
#include "hwaccess.h"
#include "LVGL_Task.h"

#define PRINT_TASK_PERIOD_MS        1000U
#define PRINT_TASK_STACK_WARN_BYTES 128U

extern volatile uint32_t g_lvgl_disp_flush_request_count;
extern volatile uint32_t g_lvgl_disp_flush_ready_count;
extern volatile uint32_t g_lvgl_disp_flush_wait_count;
extern volatile uint32_t g_lvgl_disp_flush_wait_timeout_count;
extern volatile uint32_t g_lvgl_disp_dma_callback_count;
extern volatile uint32_t g_lvgl_disp_last_wait_result;

/**
 * @brief 蓝牙调试打印任务入口。
 *
 * 本任务保持原有 LVGL 内存池串口打印，同时补充任务栈低水位预警。
 * 这里不会恢复完整任务表打印，也不会恢复 "ok\r\n" 心跳文本。
 */
void Print_Task(void *argument)
{
    char report_buffer[128];
    int report_len;
    uint32_t lvgl_phase;
    uint32_t task_index;
    uint32_t lvgl_stack_free_bytes;
    uint32_t flush_request_count;
    uint32_t flush_ready_count;
    uint32_t flush_wait_count;
    uint32_t flush_wait_timeout_count;
    uint32_t dma_callback_count;
    uint32_t flush_last_wait_result;
    uint32_t total_size;
    uint32_t free_size;
    uint32_t used_size;
    uint32_t free_biggest_size;
    uint32_t used_pct;
    uint32_t frag_pct;
    uint32_t max_used;
    uint32_t last_update_ms;
    static uint32_t last_warned_stack_bytes[FREERTOS_DEBUG_MAX_TASKS];

    (void)argument;

    if(HwAccess.power.open != NULL) {
        HwAccess.power.open();
    }

    if(HwAccess.bluetooth.init != NULL) {
        HwAccess.bluetooth.init();
    }

    if(HwAccess.bluetooth.enable != NULL) {
        HwAccess.bluetooth.enable();
    }

    for(;;) {
        /* 中文说明：
         * 这里只刷新任务栈低水位快照，不调用旧的任务列表打印逻辑，
         * 这样可以继续满足“任务监控先不打印”的约束。
         */
        FreeRTOS_Debug_UpdateStackMonitorSnapshot();

        /* 中文说明：
         * LVGL 内存监控量由 GUI/LVGL 任务更新，这里只读取快照并统一发送到串口。
         */
        total_size = g_lvgl_mem_total_size;
        free_size = g_lvgl_mem_free_size;
        free_biggest_size = g_lvgl_mem_free_biggest_size;
        used_pct = g_lvgl_mem_used_pct;
        frag_pct = g_lvgl_mem_frag_pct;
        max_used = g_lvgl_mem_max_used;
        last_update_ms = g_lvgl_mem_last_update_ms;
        used_size = (total_size >= free_size) ? (total_size - free_size) : 0U;
        lvgl_phase = g_lvgl_task_phase;
        lvgl_stack_free_bytes = 0U;
        flush_request_count = g_lvgl_disp_flush_request_count;
        flush_ready_count = g_lvgl_disp_flush_ready_count;
        flush_wait_count = g_lvgl_disp_flush_wait_count;
        flush_wait_timeout_count = g_lvgl_disp_flush_wait_timeout_count;
        dma_callback_count = g_lvgl_disp_dma_callback_count;
        flush_last_wait_result = g_lvgl_disp_last_wait_result;

        for(task_index = 0U; task_index < g_freertos_debug_task_count; task_index++) {
            const char *task_name = g_freertos_debug_tasks[task_index].name;

            if((task_name != NULL) && (strcmp(task_name, "lvglTask") == 0)) {
                lvgl_stack_free_bytes = g_freertos_debug_tasks[task_index].stack_min_free_bytes;
                break;
            }
        }

        report_len = snprintf(report_buffer,
                              sizeof(report_buffer),
                              "mem u=%lu f=%lu b=%lu p=%lu m=%lu ls=%lu lp=%lu fq=%lu fr=%lu fw=%lu ft=%lu dc=%lu wr=%lu t=%lu\r\n",
                              (unsigned long)used_size,
                              (unsigned long)free_size,
                              (unsigned long)free_biggest_size,
                              (unsigned long)used_pct,
                              (unsigned long)max_used,
                              (unsigned long)lvgl_stack_free_bytes,
                              (unsigned long)lvgl_phase,
                              (unsigned long)flush_request_count,
                              (unsigned long)flush_ready_count,
                              (unsigned long)flush_wait_count,
                              (unsigned long)flush_wait_timeout_count,
                              (unsigned long)dma_callback_count,
                              (unsigned long)flush_last_wait_result,
                              (unsigned long)last_update_ms);

        if((HwAccess.bluetooth.send != NULL) &&
           (report_len > 0) &&
           ((size_t)report_len < sizeof(report_buffer))) {
            (void)HwAccess.bluetooth.send((const uint8_t *)report_buffer,
                                          (uint16_t)report_len,
                                          50U);
        }

        if(HwAccess.bluetooth.send != NULL) {
            for(task_index = 0U; task_index < g_freertos_debug_task_count; task_index++) {
                uint32_t current_stack_free_bytes;
                const char *task_name;

                current_stack_free_bytes = g_freertos_debug_tasks[task_index].stack_min_free_bytes;
                task_name = g_freertos_debug_tasks[task_index].name;

                /* 中文说明：
                 * 只在栈低于阈值时告警；如果剩余字节数没有变化，就不重复刷屏。
                 */
                if(current_stack_free_bytes > PRINT_TASK_STACK_WARN_BYTES) {
                    last_warned_stack_bytes[task_index] = current_stack_free_bytes;
                    continue;
                }

                if(last_warned_stack_bytes[task_index] == current_stack_free_bytes) {
                    continue;
                }

                report_len = snprintf(report_buffer,
                                      sizeof(report_buffer),
                                      "stk %s=%lu\r\n",
                                      (task_name != NULL) ? task_name : "unknown",
                                      (unsigned long)current_stack_free_bytes);
                if((report_len > 0) && ((size_t)report_len < sizeof(report_buffer))) {
                    (void)HwAccess.bluetooth.send((const uint8_t *)report_buffer,
                                                  (uint16_t)report_len,
                                                  50U);
                    last_warned_stack_bytes[task_index] = current_stack_free_bytes;
                }
            }
        }

        osDelay(PRINT_TASK_PERIOD_MS);
    }
}
