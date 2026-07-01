#ifndef PPG_SIGNAL_PROCESSOR_H
#define PPG_SIGNAL_PROCESSOR_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PPG_DC_BASELINE_SHIFT               6U
#define PPG_SMOOTH_WINDOW_SIZE              5U
#define PPG_WARMUP_SAMPLE_COUNT             80U
#define PPG_GROSS_JUMP_THRESHOLD            4096U
#define PPG_MAX_TIMESTAMP_GAP_MS            75U
#define PPG_RAW_DELTA_ABS_FLOOR             450U
#define PPG_RAW_DELTA_BASELINE_DIV          10U
#define PPG_AC_ABS_FLOOR                    500U
#define PPG_AC_BASELINE_DIV                 12U

/**
 * @brief PPG 样本附加标志位。
 *
 * CSV 字段里的 flags 就是这些 bit 的组合。
 * 代码中必须通过命名宏判断，不允许直接使用魔法数字。
 */
typedef enum
{
    PPG_SAMPLE_FLAG_NONE = 0x00U,
    PPG_SAMPLE_FLAG_I2C_ERROR = 0x01U,
    PPG_SAMPLE_FLAG_RETRY_USED = 0x02U,
    PPG_SAMPLE_FLAG_RETRY_SUCCESS = 0x04U,
    PPG_SAMPLE_FLAG_GROSS_OUTLIER = 0x08U,
    PPG_SAMPLE_FLAG_WARMUP = 0x10U,
    PPG_SAMPLE_FLAG_FILTER_READY = 0x20U,
    PPG_SAMPLE_FLAG_SEQUENCE_ERROR = 0x40U,
    PPG_SAMPLE_FLAG_TIMESTAMP_ERROR = 0x80U
} PpgSampleFlags_t;

/**
 * @brief 单个前处理完成后的 PPG 样本。
 *
 * 字段顺序与串口 CSV 保持一致：
 * sequence,timestamp_ms,raw_ppg,baseline_ppg,ac_ppg,filtered_ppg,valid,flags
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
 * @brief PPG 前处理器状态。
 *
 * 阶段 1：检查 sequence / timestamp 连续性。
 * 阶段 2：维护直流基线 baseline_q8。
 * 阶段 3：计算 ac_ppg 与 filtered_ppg。
 * 阶段 4：累计拒绝原因和统计计数，供心率层与调试输出使用。
 */
typedef struct
{
    uint8_t initialized;
    uint8_t filter_ready;
    uint8_t previous_valid_raw_valid;
    uint8_t previous_sample_seen;
    uint32_t valid_sample_count;
    uint32_t invalid_sample_count;
    uint32_t warmup_sample_count;
    uint32_t gross_outlier_count;
    uint32_t raw_reject_count;
    uint32_t flag_reject_count;
    uint32_t sequence_gap_count;
    uint32_t timestamp_gap_count;
    uint32_t lost_sample_count;
    uint32_t retry_success_count;
    uint32_t previous_sequence;
    uint32_t previous_timestamp_ms;
    uint16_t previous_valid_raw;
    int32_t baseline_q8;
    int32_t last_baseline_ppg;
    int32_t last_filtered_ppg;
    int32_t smooth_buffer[PPG_SMOOTH_WINDOW_SIZE];
    int32_t smooth_sum;
    uint32_t expected_interval_ms_q8;
    uint8_t smooth_count;
    uint8_t smooth_index;
} PpgSignalProcessor_t;

/**
 * @brief 初始化 PPG 前处理器。
 *
 * @param processor PPG 前处理器实例，不允许为 NULL。
 */
void PpgSignalProcessor_Init(PpgSignalProcessor_t * processor);

/**
 * @brief 判断当前 raw_ppg 是否像毛刺，需要先重读一次再决定是否接受。（判断粗大误差）
 *
 * 这里只做快速门控，不更新任何滤波状态。
 *
 * @param processor PPG 前处理器实例，不允许为 NULL。
 * @param raw_ppg 当前原始 PPG 值。
 *
 * @return 1 是粗大误差；0 正常数据
 */
uint8_t PpgSignalProcessor_IsGrossOutlier(const PpgSignalProcessor_t * processor,
                                          uint16_t raw_ppg);

/**
 * @brief 处理一次总线读成功的原始 PPG 样本。
 *
 * 数据流：
 * raw_ppg -> 时序检查 -> 原始幅度检查 -> baseline -> ac_ppg -> filtered_ppg -> output
 *
 * 若样本在任意一步被拒绝，则该样本不能污染 baseline 与平滑滤波状态。
 *
 * @param processor PPG 前处理器实例，不允许为 NULL。
 * @param sequence 当前采样序号。
 * @param timestamp_ms 当前采样时间戳，单位 ms。
 * @param raw_ppg 当前原始 PPG 值。
 * @param flags 当前样本附加标志。
 * @param output 处理后输出样本，不允许为 NULL。
 */
void PpgSignalProcessor_ProcessValidSample(PpgSignalProcessor_t * processor,
                                           uint32_t sequence,
                                           uint32_t timestamp_ms,
                                           uint16_t raw_ppg,
                                           uint8_t flags,
                                           PpgProcessedSample_t * output);

/**
 * @brief 生成一次不能进入前处理的无效样本记录。
 *
 * 典型原因包括 I2C 失败、重读失败、时序异常等。
 * 这个接口只填写输出和计数，不更新 baseline 与平滑滤波状态。
 *
 * @param processor PPG 前处理器实例，不允许为 NULL。
 * @param sequence 当前采样序号。
 * @param timestamp_ms 当前采样时间戳，单位 ms。
 * @param flags 当前无效样本标志。
 * @param output 处理后输出样本，不允许为 NULL。
 */
void PpgSignalProcessor_ProcessInvalidSample(PpgSignalProcessor_t * processor,
                                             uint32_t sequence,
                                             uint32_t timestamp_ms,
                                             uint8_t flags,
                                             PpgProcessedSample_t * output);

#ifdef __cplusplus
}
#endif

#endif /* PPG_SIGNAL_PROCESSOR_H */
