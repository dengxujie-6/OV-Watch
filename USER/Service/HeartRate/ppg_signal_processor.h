#ifndef PPG_SIGNAL_PROCESSOR_H
#define PPG_SIGNAL_PROCESSOR_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PPG_DC_BASELINE_SHIFT      6U
#define PPG_SMOOTH_WINDOW_SIZE     5U
#define PPG_WARMUP_SAMPLE_COUNT    80U
#define PPG_GROSS_JUMP_THRESHOLD   4096U

/**
 * @brief PPG 样本附加标志位。
 */
typedef enum
{
    PPG_SAMPLE_FLAG_NONE = 0x00U,
    PPG_SAMPLE_FLAG_I2C_ERROR = 0x01U,
    PPG_SAMPLE_FLAG_RETRY_USED = 0x02U,
    PPG_SAMPLE_FLAG_GROSS_OUTLIER = 0x04U,
    PPG_SAMPLE_FLAG_WARMUP = 0x08U,
    PPG_SAMPLE_FLAG_FILTER_READY = 0x10U
} PpgSampleFlags_t;

/**
 * @brief 单个处理后的 PPG 样本。
 */
typedef struct
{
    uint32_t sequence;
    uint32_t timestamp_ms;
    uint16_t raw_ppg;
    int32_t baseline_ppg;
    int32_t ac_ppg;
    int32_t filtered_ppg;
    uint8_t valid;
    uint8_t flags;
} PpgProcessedSample_t;

/**
 * @brief PPG 原始信号处理器状态。
 */
typedef struct
{
    uint8_t initialized;
    uint8_t filter_ready;
    uint32_t valid_sample_count;
    uint32_t invalid_sample_count;
    uint32_t warmup_sample_count;
    uint32_t gross_outlier_count;
    uint32_t retry_success_count;
    uint32_t retry_fail_count;
    uint16_t previous_valid_raw;
    uint8_t previous_valid_raw_valid;
    int32_t baseline_q8;
    int32_t smooth_buffer[PPG_SMOOTH_WINDOW_SIZE];
    int32_t smooth_sum;
    uint8_t smooth_count;
    uint8_t smooth_index;
} PpgSignalProcessor_t;

/**
 * @brief 初始化 PPG 原始信号处理器。
 *
 * @param processor PPG 信号处理器实例，不允许为 NULL。
 */
void PpgSignalProcessor_Init(PpgSignalProcessor_t *processor);

/**
 * @brief 判断当前原始样本是否属于明显异常跳变。
 *
 * @param processor PPG 信号处理器实例，不允许为 NULL。
 * @param raw_ppg 当前原始 PPG 数据。
 *
 * @return uint8_t 1 表示异常跳变，0 表示正常。
 */
uint8_t PpgSignalProcessor_IsGrossOutlier(const PpgSignalProcessor_t *processor,
                                          uint16_t raw_ppg);

/**
 * @brief 处理一个有效的原始 PPG 样本。
 *
 * @param processor PPG 信号处理器实例，不允许为 NULL。
 * @param sequence 当前采样序号。
 * @param timestamp_ms 当前采样时间戳，单位 ms。
 * @param raw_ppg 当前原始 PPG 数据。
 * @param flags 当前样本附加标志。
 * @param output 处理后样本输出对象，不允许为 NULL。
 */
void PpgSignalProcessor_ProcessValidSample(PpgSignalProcessor_t *processor,
                                           uint32_t sequence,
                                           uint32_t timestamp_ms,
                                           uint16_t raw_ppg,
                                           uint8_t flags,
                                           PpgProcessedSample_t *output);

/**
 * @brief 生成一个无效 PPG 样本记录。
 *
 * @param processor PPG 信号处理器实例，不允许为 NULL。
 * @param sequence 当前采样序号。
 * @param timestamp_ms 当前采样时间戳，单位 ms。
 * @param flags 当前无效样本标志。
 * @param output 处理后样本输出对象，不允许为 NULL。
 */
void PpgSignalProcessor_ProcessInvalidSample(PpgSignalProcessor_t *processor,
                                             uint32_t sequence,
                                             uint32_t timestamp_ms,
                                             uint8_t flags,
                                             PpgProcessedSample_t *output);

#ifdef __cplusplus
}
#endif

#endif /* PPG_SIGNAL_PROCESSOR_H */
