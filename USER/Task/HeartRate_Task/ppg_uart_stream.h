#ifndef PPG_UART_STREAM_H
#define PPG_UART_STREAM_H

#include <stdint.h>

#include "FreeRTOS.h"
#include "ppg_signal_processor.h"
#include "stm32f4xx_hal.h"
#include "task.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PPG_UART_STREAM_BUFFER_COUNT    2U
#define PPG_UART_STREAM_BUFFER_SIZE     512U
#define PPG_UART_STREAM_INVALID_INDEX   0xFFU

/**
 * @brief PPG UART 双缓冲块状态。
 */
typedef enum
{
    PPG_UART_BUFFER_STATE_FREE = 0,
    PPG_UART_BUFFER_STATE_FILLING,
    PPG_UART_BUFFER_STATE_READY,
    PPG_UART_BUFFER_STATE_SENDING,
} PpgUartBufferState_t;

/**
 * @brief 单个 PPG UART 缓冲块。
 */
typedef struct
{
    uint8_t data[PPG_UART_STREAM_BUFFER_SIZE];
    uint16_t length;
    uint16_t sample_count;
    PpgUartBufferState_t state;
} PpgUartBuffer_t;

/**
 * @brief PPG 串流统计信息。
 */
typedef struct
{
    uint32_t sample_ok_count;
    uint32_t sample_i2c_error_count;
    uint32_t gross_outlier_count;
    uint32_t retry_success_count;
    uint32_t retry_fail_count;
    uint32_t valid_processed_count;
    uint32_t invalid_processed_count;
    uint32_t i2c_busy_count;
    uint32_t i2c_timeout_count;
    uint32_t i2c_hal_error_count;
    uint32_t pushed_samples;
    uint32_t sent_samples;
    uint32_t dropped_samples;
    uint32_t dma_start_count;
    uint32_t dma_complete_count;
    uint32_t dma_error_count;
    uint32_t hrs1_ctrl_mismatch_count;
    uint32_t max_timestamp_interval_ms;
    uint32_t repeated_raw_count;
    uint32_t max_same_raw_run_length;
    int32_t latest_baseline_ppg;
    int32_t latest_ac_ppg;
    int32_t latest_filtered_ppg;
    uint8_t hrs1_ctrl_expected;
    uint8_t hrs1_ctrl_readback;
} PpgUartStreamStats_t;

/**
 * @brief PPG UART 双缓冲串流对象。
 */
typedef struct
{
    PpgUartBuffer_t buffers[PPG_UART_STREAM_BUFFER_COUNT];
    TaskHandle_t tx_task_handle;
    PpgUartStreamStats_t stats;
    uint32_t next_sequence;
    uint32_t last_sample_timestamp_ms;
    uint16_t last_raw_ppg;
    uint32_t current_same_raw_run_length;
    uint8_t has_last_sample_timestamp;
    uint8_t has_last_raw_ppg;
    uint8_t filling_index;
    uint8_t sending_index;
    uint8_t dma_busy;
    uint8_t dma_complete_pending;
    uint8_t dma_error_pending;
    uint8_t flush_pending;
    uint8_t initialized;
} PpgUartStream_t;

/**
 * @brief 初始化 PPG UART 双缓冲对象。
 *
 * @param stream 串流对象，不允许为 NULL。
 */
void PpgUartStream_Init(PpgUartStream_t *stream);

/**
 * @brief 重置串流对象状态和统计信息。
 *
 * @param stream 串流对象，不允许为 NULL。
 */
void PpgUartStream_Reset(PpgUartStream_t *stream);

/**
 * @brief 绑定 UART 发送任务句柄。
 *
 * @param stream 串流对象，不允许为 NULL。
 * @param task_handle 发送任务句柄，允许为 NULL。
 */
void PpgUartStream_BindTxTask(PpgUartStream_t *stream, TaskHandle_t task_handle);

/**
 * @brief 记录一个固定采样周期，并返回当前样本 sequence。
 *
 * @param stream 串流对象，不允许为 NULL。
 * @param timestamp_ms 当前采样时间戳，单位 ms。
 *
 * @return 当前样本 sequence。
 */
uint32_t PpgUartStream_BeginSample(PpgUartStream_t *stream,
                                   uint32_t timestamp_ms);

/**
 * @brief 记录一次 I2C 采样失败。
 *
 * @param stream 串流对象，不允许为 NULL。
 * @param hal_status 最近一次 I2C 访问对应的 HAL 状态。
 * @param hal_error 最近一次 I2C 访问对应的 HAL 错误码。
 */
void PpgUartStream_RecordI2cError(PpgUartStream_t *stream,
                                  HAL_StatusTypeDef hal_status,
                                  uint32_t hal_error);

/**
 * @brief 记录一次处理后的样本统计结果。
 *
 * @param stream 串流对象，不允许为 NULL。
 * @param sample 处理后的样本，不允许为 NULL。
 */
void PpgUartStream_RecordProcessedSample(PpgUartStream_t *stream,
                                         const PpgProcessedSample_t *sample);

/**
 * @brief 记录一次明显异常跳变事件。
 *
 * @param stream 串流对象，不允许为 NULL。
 */
void PpgUartStream_RecordGrossOutlier(PpgUartStream_t *stream);

/**
 * @brief 记录一次单次重读成功。
 *
 * @param stream 串流对象，不允许为 NULL。
 */
void PpgUartStream_RecordRetrySuccess(PpgUartStream_t *stream);

/**
 * @brief 记录一次单次重读失败。
 *
 * @param stream 串流对象，不允许为 NULL。
 */
void PpgUartStream_RecordRetryFail(PpgUartStream_t *stream);

/**
 * @brief 记录 HRS1 控制寄存器的期望值和读回值。
 *
 * @param stream 串流对象，不允许为 NULL。
 * @param expected 期望寄存器值。
 * @param readback 实际读回值。
 */
void PpgUartStream_RecordHrs1Ctrl(PpgUartStream_t *stream,
                                  uint8_t expected,
                                  uint8_t readback);

/**
 * @brief 追加一条处理后的 CSV 记录。
 *
 * @param stream 串流对象，不允许为 NULL。
 * @param sample 处理后的样本，不允许为 NULL。
 *
 * @return 0 表示成功写入；负值表示缓冲不可写或格式化失败。
 */
int PpgUartStream_PushSample(PpgUartStream_t *stream,
                             const PpgProcessedSample_t *sample);

/**
 * @brief 追加一段已经格式化好的文本到 UART DMA 双缓冲。
 *
 * @param stream 串流对象，不允许为 NULL。
 * @param text 文本缓冲区，不允许为 NULL。
 * @param length 文本长度，单位字节。
 *
 * @return 0 表示成功写入；负值表示缓冲不可写。
 */
int PpgUartStream_PushText(PpgUartStream_t *stream,
                           const char *text,
                           uint16_t length);

/**
 * @brief 追加一段调试文本到 UART DMA 双缓冲，但不计入样本统计。
 *
 * @param stream 串流对象，不允许为 NULL。
 * @param text 文本缓冲区，不允许为 NULL。
 * @param length 文本长度，单位字节。
 *
 * @return 0 表示成功写入；负值表示缓冲不可写。
 */
int PpgUartStream_PushMetaText(PpgUartStream_t *stream,
                               const char *text,
                               uint16_t length);

/**
 * @brief 请求把当前未满缓冲也刷新发送。
 *
 * @param stream 串流对象，不允许为 NULL。
 */
void PpgUartStream_RequestFlush(PpgUartStream_t *stream);

/**
 * @brief 在任务上下文中推进 DMA 发送状态机。
 *
 * @param stream 串流对象，不允许为 NULL。
 */
void PpgUartStream_Process(PpgUartStream_t *stream);

/**
 * @brief DMA 完成 ISR 回调入口。
 *
 * @param context 应传入对应的 PpgUartStream_t 指针。
 */
void PpgUartStream_OnTxCompleteFromIsr(void *context);

/**
 * @brief UART 错误 ISR 回调入口。
 *
 * @param context 应传入对应的 PpgUartStream_t 指针。
 */
void PpgUartStream_OnUartErrorFromIsr(void *context);

/**
 * @brief 判断串流对象是否已经空闲。
 *
 * @param stream 串流对象，不允许为 NULL。
 *
 * @return 1 表示没有待处理数据，0 表示仍有待处理数据。
 */
uint8_t PpgUartStream_IsIdle(const PpgUartStream_t *stream);

/**
 * @brief 获取只读统计快照。
 *
 * @param stream 串流对象，不允许为 NULL。
 * @param stats 输出统计对象，不允许为 NULL。
 */
void PpgUartStream_GetStats(const PpgUartStream_t *stream,
                            PpgUartStreamStats_t *stats);

#ifdef __cplusplus
}
#endif

#endif /* PPG_UART_STREAM_H */
