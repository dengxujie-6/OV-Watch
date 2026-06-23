#ifndef HEART_RATE_TASK_H
#define HEART_RATE_TASK_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 心率监测任务入口。
 *
 * 任务默认阻塞等待运行事件组中的心率开始事件；收到开始事件后，
 * 启动 EM7028 连续采样并周期刷新 HwAccess 心率缓存，直到收到停止事件。
 *
 * @param argument FreeRTOS 任务参数，当前未使用。
 */
void HeartRate_Task(void *argument);

#ifdef __cplusplus
}
#endif

#endif /* HEART_RATE_TASK_H */
