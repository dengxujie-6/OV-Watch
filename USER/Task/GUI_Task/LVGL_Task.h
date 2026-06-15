#ifndef LVGL_TASK_H
#define LVGL_TASK_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void LVGL_Task(void *argument);
uint32_t LVGL_Task_GetHeartbeatTick(void);

#ifdef __cplusplus
}
#endif

#endif
