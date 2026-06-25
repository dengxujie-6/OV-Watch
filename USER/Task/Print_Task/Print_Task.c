#include "Print_Task.h"

#include <stdint.h>
#include <stdio.h>

#include "cmsis_os2.h"
#include "hwaccess.h"
#include "LVGL_Task.h"

#define PRINT_TASK_PERIOD_MS 1000U

/**
 * @brief 蓝牙调试打印任务入口。
 *
 * 任务启动后保持电源与蓝牙模块可用，并周期输出 LVGL 内存池监控快照。
 * 当前按用户要求：
 * - 不再打印任务监控列表；
 * - 不再打印 "ok\r\n" 心跳文本。
 */
void Print_Task(void *argument)
{
    char report_buffer[128];
    int report_len;
    uint32_t total_size;
    uint32_t free_size;
    uint32_t used_size;
    uint32_t free_biggest_size;
    uint32_t used_pct;
    uint32_t frag_pct;
    uint32_t max_used;
    uint32_t last_update_ms;

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
        // 这些监控量只由 GUI/LVGL 任务写入，这里读取快照后统一串口输出。
        total_size = g_lvgl_mem_total_size;
        free_size = g_lvgl_mem_free_size;
        free_biggest_size = g_lvgl_mem_free_biggest_size;
        used_pct = g_lvgl_mem_used_pct;
        frag_pct = g_lvgl_mem_frag_pct;
        max_used = g_lvgl_mem_max_used;
        last_update_ms = g_lvgl_mem_last_update_ms;
        used_size = (total_size >= free_size) ? (total_size - free_size) : 0U;

        report_len = snprintf(report_buffer,
                              sizeof(report_buffer),
                              "lv_mem total=%lu used=%lu free=%lu big=%lu used_pct=%lu frag=%lu max=%lu tick=%lu\r\n",
                              (unsigned long)total_size,
                              (unsigned long)used_size,
                              (unsigned long)free_size,
                              (unsigned long)free_biggest_size,
                              (unsigned long)used_pct,
                              (unsigned long)frag_pct,
                              (unsigned long)max_used,
                              (unsigned long)last_update_ms);

        if((HwAccess.bluetooth.send != NULL) &&
           (report_len > 0) &&
           ((size_t)report_len < sizeof(report_buffer))) {
            (void)HwAccess.bluetooth.send((const uint8_t *)report_buffer,
                                          (uint16_t)report_len,
                                          50U);
        }

        osDelay(PRINT_TASK_PERIOD_MS);
    }
}
