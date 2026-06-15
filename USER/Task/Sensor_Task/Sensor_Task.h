#ifndef SENSOR_TASK_H
#define SENSOR_TASK_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 传感器数据刷新任务入口。
 *
 * 任务每 500ms 通过 HwAccess 刷新一次缓存数据。当前只采样 PA1 电池电压，
 * 后续接入温湿度、心率等传感器时也应在这里刷新缓存，UI 只读缓存。
 *
 * @param argument FreeRTOS 任务参数，当前未使用。
 */
void Sensor_Task(void *argument);

#ifdef __cplusplus
}
#endif

#endif /* SENSOR_TASK_H */
