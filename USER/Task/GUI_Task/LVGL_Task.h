#ifndef LVGL_TASK_H
#define LVGL_TASK_H

#include <stdint.h>
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

void LVGL_Task(void *argument);
uint32_t LVGL_Task_GetHeartbeatTick(void);
void LVGL_Task_LogPrint(lv_log_level_t level, const char * txt);
void LVGL_Task_DebugCaptureMem(uint32_t tag);

extern volatile uint32_t g_lvgl_mem_total_size;
extern volatile uint32_t g_lvgl_mem_free_size;
extern volatile uint32_t g_lvgl_mem_free_biggest_size;
extern volatile uint32_t g_lvgl_mem_used_pct;
extern volatile uint32_t g_lvgl_mem_frag_pct;
extern volatile uint32_t g_lvgl_mem_max_used;
extern volatile uint32_t g_lvgl_mem_last_update_ms;
extern volatile uint32_t g_lvgl_mem_debug_tag;
extern volatile uint32_t g_lvgl_mem_debug_seq;
extern volatile uint32_t g_lvgl_task_phase;
extern volatile uint32_t g_lvgl_log_seq;
extern volatile uint32_t g_lvgl_log_level;
extern volatile uint32_t g_lvgl_log_last_tick;
extern char g_lvgl_log_text[96];

#ifdef __cplusplus
}
#endif

#endif
