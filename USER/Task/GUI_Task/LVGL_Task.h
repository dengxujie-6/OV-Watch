#ifndef LVGL_TASK_H
#define LVGL_TASK_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void LVGL_Task(void *argument);
uint32_t LVGL_Task_GetHeartbeatTick(void);

extern volatile uint32_t g_lvgl_mem_total_size;
extern volatile uint32_t g_lvgl_mem_free_size;
extern volatile uint32_t g_lvgl_mem_free_biggest_size;
extern volatile uint32_t g_lvgl_mem_used_pct;
extern volatile uint32_t g_lvgl_mem_frag_pct;
extern volatile uint32_t g_lvgl_mem_max_used;
extern volatile uint32_t g_lvgl_mem_last_update_ms;

#ifdef __cplusplus
}
#endif

#endif
