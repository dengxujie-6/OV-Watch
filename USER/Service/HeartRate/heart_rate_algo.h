#ifndef HEART_RATE_ALGO_H
#define HEART_RATE_ALGO_H

#include <stdint.h>

#include "ppg_signal_processor.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PPG_SAMPLE_RATE_HZ          40U
#define HR_WINDOW_SECONDS           8U
#define HR_WINDOW_SAMPLES           (PPG_SAMPLE_RATE_HZ * HR_WINDOW_SECONDS)

#define HR_MIN_BPM                  45U
#define HR_MAX_BPM                  180U

#define HR_MIN_LAG_SAMPLES          13U
#define HR_MAX_LAG_SAMPLES          53U

#define HR_ANALYZE_PERIOD_SAMPLES   10U
#define HR_MIN_SIGNAL_STD           15.0f
#define HR_MIN_AUTOCORR_PEAK        0.55f
#define HR_MIN_VALID_RATIO          0.90f
#define HR_MAX_BPM_JUMP             20.0f
#define HR_INVALID_TIMEOUT_WINDOWS  3U
#define HR_MEDIAN_WINDOW_SIZE       3U
#define HR_TIMING_GAP_MIN_MS        75U

typedef enum
{
    HR_STATE_IDLE = 0,
    HR_STATE_WARMUP,
    HR_STATE_TRACKING,
    HR_STATE_HOLD_INVALID
} HeartRateAlgoState_t;

/**
 * @brief 心率算法调试快照。
 *
 * 这些字段面向任务层调试输出和页面缓存，不直接参与算法计算。
 */
typedef struct
{
    uint32_t sequence_gap_count;
    uint32_t timestamp_gap_count;
    uint32_t lost_sample_count;
    float window_valid_ratio;
    float autocorr_peak;
    float candidate_bpm;
    float display_bpm;
    uint8_t bus_ok;
    uint8_t sample_valid;
    uint8_t hr_input_valid;
    uint8_t hr_valid;
    uint8_t display_bpm_u8;
    uint8_t state;
} HeartRateAlgoDebugInfo_t;

/**
 * @brief 心率算法状态对象。
 *
 * 数据流分为四段：
 * 1. 从前处理层接收 filtered_ppg 和样本有效性。
 * 2. 写入滑动窗口 filtered_signal[] / sample_valid_mask[]。
 * 3. 周期性做自相关分析，得到 candidate_bpm。
 * 4. 对 candidate_bpm 做稳定性与显示平滑，输出 display_bpm。
 */
typedef struct
{
    int32_t filtered_signal[HR_WINDOW_SAMPLES];
    uint8_t sample_valid_mask[HR_WINDOW_SAMPLES];
    int32_t median_history[HR_MEDIAN_WINDOW_SIZE];
    uint32_t last_sequence;
    uint32_t last_timestamp_ms;
    uint32_t expected_interval_ms_q8;
    uint32_t analyze_sample_counter;
    uint32_t total_samples_in_window;
    uint32_t valid_samples_in_window;
    uint32_t sequence_gap_count;
    uint32_t timestamp_gap_count;
    uint32_t lost_sample_count;
    float latest_candidate_bpm;
    float latest_autocorr_peak;
    float latest_window_valid_ratio;
    float stable_bpm;
    float display_bpm;
    float pending_bpm;
    uint8_t initialized;
    uint16_t write_index;
    uint8_t median_write_index;
    uint8_t median_count;
    uint8_t have_last_timing;
    uint8_t pending_match_count;
    uint8_t invalid_analysis_streak;
    uint8_t hr_valid;
    uint8_t latest_bus_ok;
    uint8_t latest_sample_valid;
    uint8_t latest_hr_input_valid;
    HeartRateAlgoState_t state;
} HeartRateAlgo_t;

/**
 * @brief 初始化心率算法对象。
 *
 * @param algo 心率算法对象，不允许为 NULL。
 */
void HeartRateAlgo_Init(HeartRateAlgo_t * algo);

/**
 * @brief 重置心率窗口，并切换到指定状态。
 *
 * @param algo 心率算法对象，不允许为 NULL。
 * @param next_state 重置后的目标状态。
 */
void HeartRateAlgo_Reset(HeartRateAlgo_t * algo, HeartRateAlgoState_t next_state);

/**
 * @brief 向心率算法输入一个已经完成前处理的 PPG 样本。
 *
 * 算法的主输入是 sample->filtered_ppg。
 * 算法层不会重复计算 baseline_ppg、ac_ppg、filtered_ppg。
 *
 * @param algo 心率算法对象，不允许为 NULL。
 * @param sample 前处理后的 PPG 样本，不允许为 NULL。
 */
void HeartRateAlgo_ProcessSample(HeartRateAlgo_t * algo,
                                 const PpgProcessedSample_t * sample);

/**
 * @brief 获取当前心率算法调试快照。
 *
 * @param algo 心率算法对象，不允许为 NULL。
 * @param info 输出快照，不允许为 NULL。
 */
void HeartRateAlgo_GetDebugInfo(const HeartRateAlgo_t * algo,
                                HeartRateAlgoDebugInfo_t * info);

#ifdef __cplusplus
}
#endif

#endif /* HEART_RATE_ALGO_H */
