#ifndef PRINT_TASK_H
#define PRINT_TASK_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 蓝牙调试打印任务入口。
 *
 * 该任务只用于最小化验证调度器与蓝牙串口链路：
 * - 任务启动后主动拉起电源保持与蓝牙模块；
 * - 之后每隔 1 秒阻塞发送一次 "ok\r\n"；
 * - 不依赖 LVGL、页面或心率串流链路。
 *
 * @param argument FreeRTOS 任务参数，当前未使用。
 */
void Print_Task(void *argument);

#ifdef __cplusplus
}
#endif

#endif /* PRINT_TASK_H */
