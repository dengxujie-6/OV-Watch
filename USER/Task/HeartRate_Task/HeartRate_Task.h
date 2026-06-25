#ifndef HEART_RATE_TASK_H
#define HEART_RATE_TASK_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief EM7028 原始 PPG 采样任务入口。
 *
 * 该任务仅在心率页面启动采样时执行固定 25ms 周期读取，并把测量原始值
 * 写入软件双缓冲，不直接执行串口发送。
 *
 * @param argument FreeRTOS 任务参数，当前未使用。
 */
void HeartRate_Task(void *argument);

/**
 * @brief 心率 UART DMA 发送任务入口。
 *
 * 该任务负责把采样任务写入的软件双缓冲通过蓝牙串口 DMA 低优先级发送出去。
 *
 * @param argument FreeRTOS 任务参数，当前未使用。
 */
void HeartRate_UartTx_Task(void *argument);

#ifdef __cplusplus
}
#endif

#endif /* HEART_RATE_TASK_H */
