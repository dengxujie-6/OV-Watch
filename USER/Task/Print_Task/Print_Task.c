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

/* 这些全局量由 GUI/LVGL 任务、显示移植层和 LVGL 内部调试埋点维护。
 * Print_Task 只读取快照并输出，不在这里修改这些状态。 */
extern volatile uint32_t g_lvgl_disp_flush_request_count;
extern volatile uint32_t g_lvgl_disp_flush_ready_count;
extern volatile uint32_t g_lvgl_disp_flush_wait_count;
extern volatile uint32_t g_lvgl_disp_flush_wait_timeout_count;
extern volatile uint32_t g_lvgl_disp_dma_callback_count;
extern volatile uint32_t g_lvgl_disp_last_wait_result;
extern volatile uint32_t g_lvgl_disp_last_flush_tick_ms;
extern volatile uint32_t g_lvgl_disp_last_flush_ready_tick_ms;
extern volatile uint32_t g_lvgl_disp_last_wait_tick_ms;
extern volatile uint32_t g_lvgl_disp_last_px_bytes;
extern volatile int32_t g_lvgl_disp_last_area_x1;
extern volatile int32_t g_lvgl_disp_last_area_y1;
extern volatile int32_t g_lvgl_disp_last_area_x2;
extern volatile int32_t g_lvgl_disp_last_area_y2;
extern volatile uint32_t g_menu_press_debug_phase;
extern volatile uint32_t g_menu_press_debug_change_count;
extern volatile uint32_t g_menu_press_debug_last_tick;
extern volatile uint32_t g_menu_press_debug_effect_on_count;
extern volatile uint32_t g_menu_press_debug_effect_off_count;
extern volatile uint32_t g_lvgl_timer_debug_phase;
extern volatile uint32_t g_lvgl_timer_debug_change_count;
extern volatile uint32_t g_lvgl_timer_debug_last_tick;
extern volatile uint32_t g_lvgl_refr_debug_phase;
extern volatile uint32_t g_lvgl_refr_debug_change_count;
extern volatile uint32_t g_lvgl_refr_debug_last_tick;
extern volatile uint32_t g_lvgl_transform_debug_phase;
extern volatile uint32_t g_lvgl_transform_debug_change_count;
extern volatile uint32_t g_lvgl_transform_debug_last_tick;
extern volatile uint32_t g_lvgl_transform_debug_layer_get_area_count;
extern volatile uint32_t g_lvgl_draw_debug_phase;
extern volatile uint32_t g_lvgl_draw_debug_change_count;
extern volatile uint32_t g_lvgl_draw_debug_last_tick;
extern volatile uint32_t g_lvgl_draw_debug_transform_loop_count;
extern volatile uint32_t g_lvgl_draw_debug_clip_corner_count;
extern volatile uint32_t g_lvgl_obj_transform_debug_phase;
extern volatile uint32_t g_lvgl_obj_transform_debug_change_count;
extern volatile uint32_t g_lvgl_obj_transform_debug_last_tick;
extern volatile uint32_t g_lvgl_obj_transform_debug_call_count;
extern volatile uint32_t g_lvgl_log_seq;
extern volatile uint32_t g_lvgl_log_level;
extern volatile uint32_t g_lvgl_log_last_tick;
extern char g_lvgl_log_text[96];

/**
 * @brief 蓝牙调试打印任务入口。
 *
 * 本任务保持原有 LVGL 内存池串口打印，同时补充任务栈低水位预警。
 * 这里不会恢复完整任务表打印，也不会恢复 "ok\r\n" 心跳文本。
 */
/**
 * @brief 调试打印任务补充说明。
 *
 * 这里会汇总几类日志：
 * - `mem`：固定周期的总览快照，用来看内存池、flush 和阶段号总体是否正常推进；
 * - `menu/core`：阶段号或关键计数变化时输出，用来对齐页面事件与 LVGL 内部链路；
 * - `lvlog`：转发最近一条 LVGL 日志，便于看到 malloc 失败、layer 分配失败等信息；
 * - `memsnap`：关键交互时刻的即时内存抓拍，用来把内存状态和具体交互动作对齐。
 */
void Print_Task(void *argument)
{
    char report_buffer[128];
    int report_len;
    uint32_t lvgl_phase;
    uint32_t task_index;
    uint32_t lvgl_stack_free_bytes;
    uint32_t menu_press_phase;
    uint32_t menu_press_change_count;
    uint32_t menu_press_last_tick;
    uint32_t menu_press_effect_on_count;
    uint32_t menu_press_effect_off_count;
    uint32_t flush_request_count;
    uint32_t flush_ready_count;
    uint32_t flush_wait_count;
    uint32_t flush_wait_timeout_count;
    uint32_t dma_callback_count;
    uint32_t flush_last_wait_result;
    uint32_t flush_last_tick_ms;
    uint32_t flush_last_ready_tick_ms;
    uint32_t flush_last_wait_tick_ms;
    uint32_t flush_last_px_bytes;
    int32_t flush_last_area_x1;
    int32_t flush_last_area_y1;
    int32_t flush_last_area_x2;
    int32_t flush_last_area_y2;
    uint32_t timer_debug_phase;
    uint32_t timer_debug_change_count;
    uint32_t timer_debug_last_tick;
    uint32_t refr_debug_phase;
    uint32_t refr_debug_change_count;
    uint32_t refr_debug_last_tick;
    uint32_t transform_debug_phase;
    uint32_t transform_debug_change_count;
    uint32_t transform_debug_last_tick;
    uint32_t transform_debug_layer_get_area_count;
    uint32_t draw_debug_phase;
    uint32_t draw_debug_change_count;
    uint32_t draw_debug_last_tick;
    uint32_t draw_debug_transform_loop_count;
    uint32_t draw_debug_clip_corner_count;
    uint32_t obj_transform_debug_phase;
    uint32_t obj_transform_debug_change_count;
    uint32_t obj_transform_debug_last_tick;
    uint32_t obj_transform_debug_call_count;
    uint32_t lvgl_log_seq;
    uint32_t lvgl_log_level;
    uint32_t lvgl_log_last_tick;
    uint32_t total_size;
    uint32_t free_size;
    uint32_t used_size;
    uint32_t free_biggest_size;
    uint32_t used_pct;
    uint32_t frag_pct;
    uint32_t max_used;
    uint32_t last_update_ms;
    uint32_t mem_debug_tag;
    uint32_t mem_debug_seq;
    static uint32_t last_warned_stack_bytes[FREERTOS_DEBUG_MAX_TASKS];
    static uint32_t s_last_menu_phase;
    static uint32_t s_last_menu_change_count;
    static uint32_t s_last_flush_request_count;
    static uint32_t s_last_flush_ready_count;
    static uint32_t s_last_flush_wait_count;
    static uint32_t s_last_flush_timeout_count;
    static uint32_t s_last_timer_debug_change_count;
    static uint32_t s_last_refr_debug_change_count;
    static uint32_t s_last_transform_debug_change_count;
    static uint32_t s_last_draw_debug_change_count;
    static uint32_t s_last_obj_transform_debug_change_count;
    static uint32_t s_last_lvgl_log_seq;
    static uint32_t s_last_mem_debug_seq;

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
        /* 先刷新一次各任务的栈监控快照，后面的低栈告警直接读取这份结果。 */
        FreeRTOS_Debug_UpdateStackMonitorSnapshot();

        /* 中文说明：
         * LVGL 内存监控量由 GUI/LVGL 任务更新，这里只读取快照并统一发送到串口。
         */
        /* 这些 LVGL 快照都由 GUI/LVGL 任务维护。
         * Print_Task 只读取并格式化输出，避免跨任务直接调用 LVGL API。 */
        total_size = g_lvgl_mem_total_size;
        free_size = g_lvgl_mem_free_size;
        free_biggest_size = g_lvgl_mem_free_biggest_size;
        used_pct = g_lvgl_mem_used_pct;
        frag_pct = g_lvgl_mem_frag_pct;
        max_used = g_lvgl_mem_max_used;
        last_update_ms = g_lvgl_mem_last_update_ms;
        mem_debug_tag = g_lvgl_mem_debug_tag;
        mem_debug_seq = g_lvgl_mem_debug_seq;
        used_size = (total_size >= free_size) ? (total_size - free_size) : 0U;
        lvgl_phase = g_lvgl_task_phase;
        lvgl_stack_free_bytes = 0U;
        menu_press_phase = g_menu_press_debug_phase;
        menu_press_change_count = g_menu_press_debug_change_count;
        menu_press_last_tick = g_menu_press_debug_last_tick;
        menu_press_effect_on_count = g_menu_press_debug_effect_on_count;
        menu_press_effect_off_count = g_menu_press_debug_effect_off_count;
        flush_request_count = g_lvgl_disp_flush_request_count;
        flush_ready_count = g_lvgl_disp_flush_ready_count;
        flush_wait_count = g_lvgl_disp_flush_wait_count;
        flush_wait_timeout_count = g_lvgl_disp_flush_wait_timeout_count;
        dma_callback_count = g_lvgl_disp_dma_callback_count;
        flush_last_wait_result = g_lvgl_disp_last_wait_result;
        flush_last_tick_ms = g_lvgl_disp_last_flush_tick_ms;
        flush_last_ready_tick_ms = g_lvgl_disp_last_flush_ready_tick_ms;
        flush_last_wait_tick_ms = g_lvgl_disp_last_wait_tick_ms;
        flush_last_px_bytes = g_lvgl_disp_last_px_bytes;
        flush_last_area_x1 = g_lvgl_disp_last_area_x1;
        flush_last_area_y1 = g_lvgl_disp_last_area_y1;
        flush_last_area_x2 = g_lvgl_disp_last_area_x2;
        flush_last_area_y2 = g_lvgl_disp_last_area_y2;
        timer_debug_phase = g_lvgl_timer_debug_phase;
        timer_debug_change_count = g_lvgl_timer_debug_change_count;
        timer_debug_last_tick = g_lvgl_timer_debug_last_tick;
        refr_debug_phase = g_lvgl_refr_debug_phase;
        refr_debug_change_count = g_lvgl_refr_debug_change_count;
        refr_debug_last_tick = g_lvgl_refr_debug_last_tick;
        transform_debug_phase = g_lvgl_transform_debug_phase;
        transform_debug_change_count = g_lvgl_transform_debug_change_count;
        transform_debug_last_tick = g_lvgl_transform_debug_last_tick;
        transform_debug_layer_get_area_count = g_lvgl_transform_debug_layer_get_area_count;
        draw_debug_phase = g_lvgl_draw_debug_phase;
        draw_debug_change_count = g_lvgl_draw_debug_change_count;
        draw_debug_last_tick = g_lvgl_draw_debug_last_tick;
        draw_debug_transform_loop_count = g_lvgl_draw_debug_transform_loop_count;
        draw_debug_clip_corner_count = g_lvgl_draw_debug_clip_corner_count;
        obj_transform_debug_phase = g_lvgl_obj_transform_debug_phase;
        obj_transform_debug_change_count = g_lvgl_obj_transform_debug_change_count;
        obj_transform_debug_last_tick = g_lvgl_obj_transform_debug_last_tick;
        obj_transform_debug_call_count = g_lvgl_obj_transform_debug_call_count;
        lvgl_log_seq = g_lvgl_log_seq;
        lvgl_log_level = g_lvgl_log_level;
        lvgl_log_last_tick = g_lvgl_log_last_tick;

        for(task_index = 0U; task_index < g_freertos_debug_task_count; task_index++) {
            const char *task_name = g_freertos_debug_tasks[task_index].name;

            if((task_name != NULL) && (strcmp(task_name, "lvglTask") == 0)) {
                lvgl_stack_free_bytes = g_freertos_debug_tasks[task_index].stack_min_free_bytes;
                break;
            }
        }

        /* `mem` 是固定周期总览：
         * 看 LVGL 内存池总体水位、碎片率、flush 推进情况，以及几个核心阶段号当前停在哪里。 */
        report_len = snprintf(report_buffer,
                              sizeof(report_buffer),
                              "mem u=%lu f=%lu b=%lu p=%lu g=%lu m=%lu ls=%lu lp=%lu mp=%lu fq=%lu fr=%lu fw=%lu ft=%lu dc=%lu wr=%lu t=%lu tp=%lu rp=%lu xp=%lu dp=%lu op=%lu dl=%lu dk=%lu\r\n",
                              (unsigned long)used_size,
                              (unsigned long)free_size,
                              (unsigned long)free_biggest_size,
                              (unsigned long)used_pct,
                              (unsigned long)frag_pct,
                              (unsigned long)max_used,
                              (unsigned long)lvgl_stack_free_bytes,
                              (unsigned long)lvgl_phase,
                              (unsigned long)menu_press_phase,
                              (unsigned long)flush_request_count,
                              (unsigned long)flush_ready_count,
                              (unsigned long)flush_wait_count,
                              (unsigned long)flush_wait_timeout_count,
                              (unsigned long)dma_callback_count,
                              (unsigned long)flush_last_wait_result,
                              (unsigned long)last_update_ms,
                              (unsigned long)timer_debug_phase,
                              (unsigned long)refr_debug_phase,
                              (unsigned long)transform_debug_phase,
                              (unsigned long)draw_debug_phase,
                              (unsigned long)obj_transform_debug_phase,
                              (unsigned long)draw_debug_transform_loop_count,
                              (unsigned long)draw_debug_clip_corner_count);

        if((HwAccess.bluetooth.send != NULL) &&
           (report_len > 0) &&
           ((size_t)report_len < sizeof(report_buffer))) {
            (void)HwAccess.bluetooth.send((const uint8_t *)report_buffer,
                                          (uint16_t)report_len,
                                          50U);
        }

        /* `menu/core` 只在阶段号或关键计数变化时补发，避免每个周期都重复刷同样的信息。 */
        if((HwAccess.bluetooth.send != NULL) &&
           ((menu_press_phase != s_last_menu_phase) ||
            (menu_press_change_count != s_last_menu_change_count) ||
            (flush_wait_timeout_count != s_last_flush_timeout_count) ||
            (timer_debug_change_count != s_last_timer_debug_change_count) ||
            (refr_debug_change_count != s_last_refr_debug_change_count) ||
            (transform_debug_change_count != s_last_transform_debug_change_count) ||
            (draw_debug_change_count != s_last_draw_debug_change_count) ||
            (obj_transform_debug_change_count != s_last_obj_transform_debug_change_count))) {
            report_len = snprintf(report_buffer,
                                  sizeof(report_buffer),
                                  "menu ph=%lu ch=%lu ton=%lu tof=%lu lt=%lu lp=%lu dq=%lu dr=%lu dw=%lu dt=%lu px=%lu a=%ld,%ld,%ld,%ld ft=%lu frt=%lu fwt=%lu wr=%lu\r\n",
                                  (unsigned long)menu_press_phase,
                                  (unsigned long)menu_press_change_count,
                                  (unsigned long)menu_press_effect_on_count,
                                  (unsigned long)menu_press_effect_off_count,
                                  (unsigned long)menu_press_last_tick,
                                  (unsigned long)lvgl_phase,
                                  (unsigned long)(flush_request_count - s_last_flush_request_count),
                                  (unsigned long)(flush_ready_count - s_last_flush_ready_count),
                                  (unsigned long)(flush_wait_count - s_last_flush_wait_count),
                                  (unsigned long)(flush_wait_timeout_count - s_last_flush_timeout_count),
                                  (unsigned long)flush_last_px_bytes,
                                  (long)flush_last_area_x1,
                                  (long)flush_last_area_y1,
                                  (long)flush_last_area_x2,
                                  (long)flush_last_area_y2,
                                  (unsigned long)flush_last_tick_ms,
                                  (unsigned long)flush_last_ready_tick_ms,
                                  (unsigned long)flush_last_wait_tick_ms,
                                  (unsigned long)flush_last_wait_result);
            if((report_len > 0) && ((size_t)report_len < sizeof(report_buffer))) {
                (void)HwAccess.bluetooth.send((const uint8_t *)report_buffer,
                                              (uint16_t)report_len,
                                              50U);
            }

            report_len = snprintf(report_buffer,
                                  sizeof(report_buffer),
                                  "core tp=%lu tc=%lu tt=%lu rp=%lu rc=%lu rt=%lu xp=%lu xc=%lu xt=%lu xl=%lu dp=%lu dc=%lu dt=%lu dl=%lu dk=%lu op=%lu oc=%lu ot=%lu on=%lu\r\n",
                                  (unsigned long)timer_debug_phase,
                                  (unsigned long)timer_debug_change_count,
                                  (unsigned long)timer_debug_last_tick,
                                  (unsigned long)refr_debug_phase,
                                  (unsigned long)refr_debug_change_count,
                                  (unsigned long)refr_debug_last_tick,
                                  (unsigned long)transform_debug_phase,
                                  (unsigned long)transform_debug_change_count,
                                  (unsigned long)transform_debug_last_tick,
                                  (unsigned long)transform_debug_layer_get_area_count,
                                  (unsigned long)draw_debug_phase,
                                  (unsigned long)draw_debug_change_count,
                                  (unsigned long)draw_debug_last_tick,
                                  (unsigned long)draw_debug_transform_loop_count,
                                  (unsigned long)draw_debug_clip_corner_count,
                                  (unsigned long)obj_transform_debug_phase,
                                  (unsigned long)obj_transform_debug_change_count,
                                  (unsigned long)obj_transform_debug_last_tick,
                                  (unsigned long)obj_transform_debug_call_count);
            if((report_len > 0) && ((size_t)report_len < sizeof(report_buffer))) {
                (void)HwAccess.bluetooth.send((const uint8_t *)report_buffer,
                                              (uint16_t)report_len,
                                              50U);
            }
        }

        s_last_menu_phase = menu_press_phase;
        s_last_menu_change_count = menu_press_change_count;
        s_last_flush_request_count = flush_request_count;
        s_last_flush_ready_count = flush_ready_count;
        s_last_flush_wait_count = flush_wait_count;
        s_last_flush_timeout_count = flush_wait_timeout_count;
        s_last_timer_debug_change_count = timer_debug_change_count;
        s_last_refr_debug_change_count = refr_debug_change_count;
        s_last_transform_debug_change_count = transform_debug_change_count;
        s_last_draw_debug_change_count = draw_debug_change_count;
        s_last_obj_transform_debug_change_count = obj_transform_debug_change_count;

        /* `lvlog` 转发最近一条 LVGL 日志，重点看 malloc/layer/draw buffer 失败。 */
        if((HwAccess.bluetooth.send != NULL) && (lvgl_log_seq != s_last_lvgl_log_seq)) {
            report_len = snprintf(report_buffer,
                                  sizeof(report_buffer),
                                  "lvlog s=%lu l=%lu t=%lu %s\r\n",
                                  (unsigned long)lvgl_log_seq,
                                  (unsigned long)lvgl_log_level,
                                  (unsigned long)lvgl_log_last_tick,
                                  g_lvgl_log_text);
            if((report_len > 0) && ((size_t)report_len < sizeof(report_buffer))) {
                (void)HwAccess.bluetooth.send((const uint8_t *)report_buffer,
                                              (uint16_t)report_len,
                                              50U);
            }
            s_last_lvgl_log_seq = lvgl_log_seq;
        }

        /* `memsnap` 是即时抓拍，不是固定周期采样。
         * 它由菜单事件等调试点主动触发，用来把“当时的内存状态”和“具体交互动作”对齐。 */
        if((HwAccess.bluetooth.send != NULL) && (mem_debug_seq != s_last_mem_debug_seq)) {
            report_len = snprintf(report_buffer,
                                  sizeof(report_buffer),
                                  "memsnap s=%lu tag=%lu u=%lu f=%lu b=%lu p=%lu g=%lu m=%lu t=%lu mp=%lu lp=%lu\r\n",
                                  (unsigned long)mem_debug_seq,
                                  (unsigned long)mem_debug_tag,
                                  (unsigned long)used_size,
                                  (unsigned long)free_size,
                                  (unsigned long)free_biggest_size,
                                  (unsigned long)used_pct,
                                  (unsigned long)frag_pct,
                                  (unsigned long)max_used,
                                  (unsigned long)last_update_ms,
                                  (unsigned long)menu_press_phase,
                                  (unsigned long)lvgl_phase);
            if((report_len > 0) && ((size_t)report_len < sizeof(report_buffer))) {
                (void)HwAccess.bluetooth.send((const uint8_t *)report_buffer,
                                              (uint16_t)report_len,
                                              50U);
            }
            s_last_mem_debug_seq = mem_debug_seq;
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
                /* 只在栈低于阈值且剩余值发生变化时告警，避免重复输出同一条低栈日志。 */
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
