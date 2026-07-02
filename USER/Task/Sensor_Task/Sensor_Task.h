#ifndef SENSOR_TASK_H
#define SENSOR_TASK_H

#include <stdint.h>

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

/**
 * @brief 在 ISR 中通知 Sensor_Task 收到一次 MPU6050 INT。
 *
 * 该接口只投递任务标志，不做阻塞操作，可在 EXTI 回调中调用。
 */
void Sensor_Task_NotifyMpuInterruptFromISR(void);

/**
 * @brief 在 STOP 唤醒后执行一次抬腕判定。
 *
 * @param window_ms 抬腕观察窗口，单位毫秒。
 *
 * @return 1 表示抬腕成立，应恢复亮屏；0 表示判定失败，可继续进入 STOP。
 */
uint8_t Sensor_Task_EvaluateRaiseWake(uint32_t window_ms);

/**
 * @brief 在明确接受一次唤醒后，强制恢复活动采样模式。
 */
void Sensor_Task_ForceActiveMode(void);

#ifdef __cplusplus
}
#endif

#endif /* SENSOR_TASK_H */
