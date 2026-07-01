#ifndef HEART_RATE_TASK_H
#define HEART_RATE_TASK_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief EM7028 原始 PPG 采样任务入口。
 *
 * 该任务在心率页面启动采样后执行固定 25ms 周期读取，
 * 完成前处理、心率算法计算和页面缓存更新。
 *
 * @param argument FreeRTOS 任务参数，当前未使用。
 */
void HeartRate_Task(void *argument);

#ifdef __cplusplus
}
#endif

#endif /* HEART_RATE_TASK_H */
