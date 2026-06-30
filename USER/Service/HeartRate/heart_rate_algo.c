#include "heart_rate_algo.h"

#include <math.h>
#include <string.h>

/**
 * @file heart_rate_algo.c
 * @brief 心率算法层。
 *
 * 这一层不再负责 raw_ppg、baseline_ppg、ac_ppg、filtered_ppg 的生成，
 * 只消费前处理层给出的 filtered_ppg 与有效性标志。
 *
 * 总数据流：
 * 阶段 1：检查 sequence / timestamp 连续性。
 * 阶段 2：把 filtered_ppg 写入滑动窗口。
 * 阶段 3：周期性执行自相关分析，得到 candidate_bpm。
 * 阶段 4：对 candidate_bpm 做稳定性门控和平滑，得到 display_bpm。
 */

typedef struct
{
    float bpm_candidate;
    float autocorr_peak;
    float valid_ratio;
    float signal_std;
    uint8_t valid;
} HeartRateAlgoAnalysis_t;

static uint8_t HeartRateAlgo_ShouldResetForSequence(const HeartRateAlgo_t * algo,
                                                    uint32_t sequence,
                                                    uint32_t * lost_samples);
static uint8_t HeartRateAlgo_ShouldResetForTimestamp(const HeartRateAlgo_t * algo,
                                                     uint32_t timestamp_ms);
static void HeartRateAlgo_UpdateExpectedInterval(HeartRateAlgo_t * algo,
                                                 uint32_t delta_ms);
static void HeartRateAlgo_PushWindowSample(HeartRateAlgo_t * algo,
                                           int32_t filtered_ppg,
                                           uint8_t valid);
static int32_t HeartRateAlgo_ApplyMedian3(HeartRateAlgo_t * algo,
                                          int32_t filtered_ppg);
static void HeartRateAlgo_RunAnalysis(HeartRateAlgo_t * algo);
static void HeartRateAlgo_SetWarmupState(HeartRateAlgo_t * algo);
static void HeartRateAlgo_RecordTiming(HeartRateAlgo_t * algo,
                                       uint32_t sequence,
                                       uint32_t timestamp_ms);
static uint32_t HeartRateAlgo_GetWindowIndex(const HeartRateAlgo_t * algo,
                                             uint32_t logical_index);
static float HeartRateAlgo_GetGapThresholdMs(const HeartRateAlgo_t * algo);
static float HeartRateAlgo_ParabolicRefineLag(float left_corr,
                                              float center_corr,
                                              float right_corr);

void HeartRateAlgo_Init(HeartRateAlgo_t * algo)
{
    if(algo == NULL) {
        return;
    }

    memset(algo, 0, sizeof(*algo));
    algo->initialized = 1U;
    algo->state = HR_STATE_IDLE;
}

void HeartRateAlgo_Reset(HeartRateAlgo_t * algo, HeartRateAlgoState_t next_state)
{
    if(algo == NULL) {
        return;
    }

    // Reset 只清窗口和算法状态，不改 initialized 标记。
    memset(algo->filtered_signal, 0, sizeof(algo->filtered_signal));
    memset(algo->sample_valid_mask, 0, sizeof(algo->sample_valid_mask));
    memset(algo->median_history, 0, sizeof(algo->median_history));
    algo->analyze_sample_counter = 0U;
    algo->total_samples_in_window = 0U;
    algo->valid_samples_in_window = 0U;
    algo->latest_candidate_bpm = 0.0f;
    algo->latest_autocorr_peak = 0.0f;
    algo->latest_window_valid_ratio = 0.0f;
    algo->stable_bpm = 0.0f;
    algo->display_bpm = 0.0f;
    algo->pending_bpm = 0.0f;
    algo->last_sequence = 0U;
    algo->last_timestamp_ms = 0U;
    algo->expected_interval_ms_q8 = 0U;
    algo->write_index = 0U;
    algo->median_write_index = 0U;
    algo->median_count = 0U;
    algo->have_last_timing = 0U;
    algo->pending_match_count = 0U;
    algo->invalid_analysis_streak = 0U;
    algo->hr_valid = 0U;
    algo->latest_bus_ok = 0U;
    algo->latest_sample_valid = 0U;
    algo->latest_hr_input_valid = 0U;
    algo->state = next_state;
}

void HeartRateAlgo_ProcessSample(HeartRateAlgo_t * algo,
                                 const PpgProcessedSample_t * sample)
{
    uint32_t lost_samples = 0U;
    uint8_t sample_valid;
    uint8_t hr_input_valid;
    uint8_t filter_ready;
    uint8_t must_reset_window = 0U;

    if((algo == NULL) || (sample == NULL)) {
        return;
    }

    if(algo->initialized == 0U) {
        HeartRateAlgo_Init(algo);
    }

    // 阶段 0：先把本样本在不同层面的有效性拆开记录，供调试摘要使用。
    algo->latest_bus_ok = ((sample->flags & PPG_SAMPLE_FLAG_I2C_ERROR) == 0U) ? 1U : 0U;
    sample_valid = sample->valid;
    algo->latest_sample_valid = sample_valid;
    algo->latest_hr_input_valid = 0U;

    if(algo->state == HR_STATE_IDLE) {
        algo->state = HR_STATE_WARMUP;
    }

    // 阶段 1：时序连续性检查。
    if(algo->have_last_timing != 0U) {
        if(HeartRateAlgo_ShouldResetForSequence(algo, sample->sequence, &lost_samples) != 0U) {
            algo->sequence_gap_count++;
            if(sample->sequence > algo->last_sequence) {
                algo->lost_sample_count += lost_samples;
                HeartRateAlgo_Reset(algo, HR_STATE_WARMUP);
                must_reset_window = 1U;
            } else {
                HeartRateAlgo_Reset(algo, HR_STATE_WARMUP);
                HeartRateAlgo_RecordTiming(algo, sample->sequence, sample->timestamp_ms);
                return;
            }
        }

        if(HeartRateAlgo_ShouldResetForTimestamp(algo, sample->timestamp_ms) != 0U) {
            algo->timestamp_gap_count++;
            if(sample->timestamp_ms <= algo->last_timestamp_ms) {
                HeartRateAlgo_Reset(algo, HR_STATE_WARMUP);
                HeartRateAlgo_RecordTiming(algo, sample->sequence, sample->timestamp_ms);
                return;
            }

            HeartRateAlgo_Reset(algo, HR_STATE_WARMUP);
            must_reset_window = 1U;
        }
    }

    // 只允许单点缺样通过插入一个 invalid 槽位来维持窗口节拍。
    if((must_reset_window == 0U) && (lost_samples == 1U)) {
        HeartRateAlgo_PushWindowSample(algo, 0, 0U);
    }

    // 阶段 2：判断 filtered_ppg 是否有资格进入心率窗口。
    filter_ready = ((sample->flags & PPG_SAMPLE_FLAG_FILTER_READY) != 0U) ? 1U : 0U;
    hr_input_valid = sample_valid;
    if((filter_ready == 0U) ||
       ((sample->flags & (PPG_SAMPLE_FLAG_I2C_ERROR |
                          PPG_SAMPLE_FLAG_GROSS_OUTLIER |
                          PPG_SAMPLE_FLAG_SEQUENCE_ERROR |
                          PPG_SAMPLE_FLAG_TIMESTAMP_ERROR)) != 0U)) {
        hr_input_valid = 0U;
    }

    // 阶段 2-1：有效点先做 median3，进一步压制孤立尖刺。
    if(hr_input_valid != 0U) {
        HeartRateAlgo_PushWindowSample(algo,
                                       HeartRateAlgo_ApplyMedian3(algo, sample->filtered_ppg),
                                       1U);
    } else {
        HeartRateAlgo_PushWindowSample(algo, 0, 0U);
    }

    algo->latest_hr_input_valid = hr_input_valid;
    HeartRateAlgo_RecordTiming(algo, sample->sequence, sample->timestamp_ms);

    // 阶段 3：按固定节拍触发整窗分析，不必每个样本都跑一次。
    algo->analyze_sample_counter++;
    if(algo->analyze_sample_counter >= HR_ANALYZE_PERIOD_SAMPLES) {
        algo->analyze_sample_counter = 0U;
        HeartRateAlgo_RunAnalysis(algo);
    }
}

void HeartRateAlgo_GetDebugInfo(const HeartRateAlgo_t * algo,
                                HeartRateAlgoDebugInfo_t * info)
{
    if((algo == NULL) || (info == NULL)) {
        return;
    }

    info->sequence_gap_count = algo->sequence_gap_count;
    info->timestamp_gap_count = algo->timestamp_gap_count;
    info->lost_sample_count = algo->lost_sample_count;
    info->window_valid_ratio = algo->latest_window_valid_ratio;
    info->autocorr_peak = algo->latest_autocorr_peak;
    info->candidate_bpm = algo->latest_candidate_bpm;
    info->display_bpm = algo->display_bpm;
    info->bus_ok = algo->latest_bus_ok;
    info->sample_valid = algo->latest_sample_valid;
    info->hr_input_valid = algo->latest_hr_input_valid;
    info->hr_valid = algo->hr_valid;
    info->display_bpm_u8 = (uint8_t)(algo->display_bpm + 0.5f);
    info->state = (uint8_t)algo->state;
}

static uint8_t HeartRateAlgo_ShouldResetForSequence(const HeartRateAlgo_t * algo,
                                                    uint32_t sequence,
                                                    uint32_t * lost_samples)
{
    if(lost_samples != NULL) {
        *lost_samples = 0U;
    }

    if(algo->have_last_timing == 0U) {
        return 0U;
    }

    if(sequence == (algo->last_sequence + 1U)) {
        return 0U;
    }

    if(sequence == (algo->last_sequence + 2U)) {
        if(lost_samples != NULL) {
            *lost_samples = 1U;
        }
        return 0U;
    }

    if((sequence > algo->last_sequence) && (lost_samples != NULL)) {
        *lost_samples = sequence - algo->last_sequence - 1U;
    }

    return 1U;
}

static uint8_t HeartRateAlgo_ShouldResetForTimestamp(const HeartRateAlgo_t * algo,
                                                     uint32_t timestamp_ms)
{
    uint32_t delta_ms;

    if(algo->have_last_timing == 0U) {
        return 0U;
    }

    if(timestamp_ms <= algo->last_timestamp_ms) {
        return 1U;
    }

    delta_ms = timestamp_ms - algo->last_timestamp_ms;
    return (delta_ms > (uint32_t)HeartRateAlgo_GetGapThresholdMs(algo)) ? 1U : 0U;
}

static void HeartRateAlgo_UpdateExpectedInterval(HeartRateAlgo_t * algo,
                                                 uint32_t delta_ms)
{
    uint32_t target_q8;

    if(algo == NULL) {
        return;
    }

    target_q8 = delta_ms << 8U;
    if(algo->expected_interval_ms_q8 == 0U) {
        algo->expected_interval_ms_q8 = target_q8;
    } else {
        algo->expected_interval_ms_q8 =
            (algo->expected_interval_ms_q8 * 7U + target_q8) / 8U;
    }
}

static void HeartRateAlgo_PushWindowSample(HeartRateAlgo_t * algo,
                                           int32_t filtered_ppg,
                                           uint8_t valid)
{
    uint32_t index;

    if(algo == NULL) {
        return;
    }

    // 环形滑窗：write_index 始终指向“下一笔待写入位置”。
    index = algo->write_index;
    if(algo->total_samples_in_window >= HR_WINDOW_SAMPLES) {
        if(algo->sample_valid_mask[index] != 0U) {
            algo->valid_samples_in_window--;
        }
    } else {
        algo->total_samples_in_window++;
    }

    algo->filtered_signal[index] = filtered_ppg;
    algo->sample_valid_mask[index] = valid;
    if(valid != 0U) {
        algo->valid_samples_in_window++;
    }

    algo->write_index++;
    if(algo->write_index >= HR_WINDOW_SAMPLES) {
        algo->write_index = 0U;
    }
}

static int32_t HeartRateAlgo_ApplyMedian3(HeartRateAlgo_t * algo,
                                          int32_t filtered_ppg)
{
    int32_t a;
    int32_t b;
    int32_t c;
    // median3 是一个极轻量的去尖峰处理，不改变主周期结构。
    if(algo == NULL) {
        return filtered_ppg;
    }

    algo->median_history[algo->median_write_index] = filtered_ppg;
    algo->median_write_index++;
    if(algo->median_write_index >= HR_MEDIAN_WINDOW_SIZE) {
        algo->median_write_index = 0U;
    }

    if(algo->median_count < HR_MEDIAN_WINDOW_SIZE) {
        algo->median_count++;
        return filtered_ppg;
    }

    a = algo->median_history[0];
    b = algo->median_history[1];
    c = algo->median_history[2];

    if(a > b) {
        int32_t temp = a;
        a = b;
        b = temp;
    }
    if(b > c) {
        int32_t temp = b;
        b = c;
        c = temp;
    }
    if(a > b) {
        int32_t temp = a;
        a = b;
        b = temp;
    }

    return b;
}

static void HeartRateAlgo_RunAnalysis(HeartRateAlgo_t * algo)
{
    HeartRateAlgoAnalysis_t analysis;
    uint32_t valid_required;

    if(algo == NULL) {
        return;
    }

    analysis.bpm_candidate = 0.0f;
    analysis.autocorr_peak = 0.0f;
    analysis.valid_ratio = 0.0f;
    analysis.signal_std = 0.0f;
    analysis.valid = 0U;

    // 阶段 3-0：窗口还没装满，观察时长不够，继续预热。
    if(algo->total_samples_in_window < HR_WINDOW_SAMPLES) {
        HeartRateAlgo_SetWarmupState(algo);
        return;
    }

    // 阶段 3-1：先看窗口内有效点比例，无效点太多则整窗不可用。
    analysis.valid_ratio = (float)algo->valid_samples_in_window / (float)HR_WINDOW_SAMPLES;
    algo->latest_window_valid_ratio = analysis.valid_ratio;
    valid_required = (uint32_t)(HR_WINDOW_SAMPLES * HR_MIN_VALID_RATIO);
    if((analysis.valid_ratio < HR_MIN_VALID_RATIO) ||
       (algo->valid_samples_in_window < valid_required)) {
        HeartRateAlgo_SetWarmupState(algo);
        return;
    }

    {
        uint32_t i;
        uint32_t valid_count = 0U;
        float mean = 0.0f;
        float variance_sum = 0.0f;
        float best_corr = -1.0f;
        uint32_t best_lag = 0U;
        float left_corr = 0.0f;
        float right_corr = 0.0f;

        // 阶段 3-2：先算均值，把波形平移到以 0 为中心。
        for(i = 0U; i < HR_WINDOW_SAMPLES; i++) {
            uint32_t idx = HeartRateAlgo_GetWindowIndex(algo, i);
            if(algo->sample_valid_mask[idx] != 0U) {
                mean += (float)algo->filtered_signal[idx];
                valid_count++;
            }
        }

        if(valid_count < valid_required) {
            HeartRateAlgo_SetWarmupState(algo);
            return;
        }

        mean /= (float)valid_count;

        // 阶段 3-3：再算标准差，排除几乎没有脉搏起伏的平线数据。
        for(i = 0U; i < HR_WINDOW_SAMPLES; i++) {
            uint32_t idx = HeartRateAlgo_GetWindowIndex(algo, i);
            float centered;

            if(algo->sample_valid_mask[idx] == 0U) {
                continue;
            }

            centered = (float)algo->filtered_signal[idx] - mean;
            variance_sum += centered * centered;
        }

        analysis.signal_std = sqrtf(variance_sum / (float)valid_count);
        if(analysis.signal_std < HR_MIN_SIGNAL_STD) {
            HeartRateAlgo_SetWarmupState(algo);
            return;
        }

        // 阶段 3-4：在允许的心率范围内搜索自相关主峰。
        for(i = HR_MIN_LAG_SAMPLES; i <= HR_MAX_LAG_SAMPLES; i++) {
            uint32_t pair_count = 0U;
            uint32_t j;
            float numerator = 0.0f;
            float denom_left = 0.0f;
            float denom_right = 0.0f;
            float corr;

            for(j = i; j < HR_WINDOW_SAMPLES; j++) {
                uint32_t idx_now = HeartRateAlgo_GetWindowIndex(algo, j);
                uint32_t idx_prev = HeartRateAlgo_GetWindowIndex(algo, j - i);
                float left;
                float right;

                if((algo->sample_valid_mask[idx_now] == 0U) ||
                   (algo->sample_valid_mask[idx_prev] == 0U)) {
                    continue;
                }

                left = (float)algo->filtered_signal[idx_now] - mean;
                right = (float)algo->filtered_signal[idx_prev] - mean;
                numerator += left * right;
                denom_left += left * left;
                denom_right += right * right;
                pair_count++;
            }

            if((pair_count < (HR_WINDOW_SAMPLES / 2U)) ||
               (denom_left <= 0.0f) ||
               (denom_right <= 0.0f)) {
                continue;
            }

            corr = numerator / sqrtf(denom_left * denom_right);
            if(corr > best_corr) {
                best_corr = corr;
                best_lag = i;
            }
        }

        // 阶段 3-5：主峰太弱或落在范围外，则这一窗不输出心率。
        if((best_lag < HR_MIN_LAG_SAMPLES) ||
           (best_lag > HR_MAX_LAG_SAMPLES) ||
           (best_corr < HR_MIN_AUTOCORR_PEAK)) {
            HeartRateAlgo_SetWarmupState(algo);
            algo->latest_autocorr_peak = (best_corr > 0.0f) ? best_corr : 0.0f;
            return;
        }

        if(best_lag > HR_MIN_LAG_SAMPLES) {
            uint32_t i = best_lag - 1U;
            uint32_t pair_count = 0U;
            uint32_t j;
            float numerator = 0.0f;
            float denom_left = 0.0f;
            float denom_right = 0.0f;

            for(j = i; j < HR_WINDOW_SAMPLES; j++) {
                uint32_t idx_now = HeartRateAlgo_GetWindowIndex(algo, j);
                uint32_t idx_prev = HeartRateAlgo_GetWindowIndex(algo, j - i);
                float left;
                float right;

                if((algo->sample_valid_mask[idx_now] == 0U) ||
                   (algo->sample_valid_mask[idx_prev] == 0U)) {
                    continue;
                }

                left = (float)algo->filtered_signal[idx_now] - mean;
                right = (float)algo->filtered_signal[idx_prev] - mean;
                numerator += left * right;
                denom_left += left * left;
                denom_right += right * right;
                pair_count++;
            }

            if((pair_count >= (HR_WINDOW_SAMPLES / 2U)) &&
               (denom_left > 0.0f) &&
               (denom_right > 0.0f)) {
                left_corr = numerator / sqrtf(denom_left * denom_right);
            } else {
                left_corr = best_corr;
            }
        } else {
            left_corr = best_corr;
        }

        if(best_lag < HR_MAX_LAG_SAMPLES) {
            uint32_t i = best_lag + 1U;
            uint32_t pair_count = 0U;
            uint32_t j;
            float numerator = 0.0f;
            float denom_left = 0.0f;
            float denom_right = 0.0f;

            for(j = i; j < HR_WINDOW_SAMPLES; j++) {
                uint32_t idx_now = HeartRateAlgo_GetWindowIndex(algo, j);
                uint32_t idx_prev = HeartRateAlgo_GetWindowIndex(algo, j - i);
                float left;
                float right;

                if((algo->sample_valid_mask[idx_now] == 0U) ||
                   (algo->sample_valid_mask[idx_prev] == 0U)) {
                    continue;
                }

                left = (float)algo->filtered_signal[idx_now] - mean;
                right = (float)algo->filtered_signal[idx_prev] - mean;
                numerator += left * right;
                denom_left += left * left;
                denom_right += right * right;
                pair_count++;
            }

            if((pair_count >= (HR_WINDOW_SAMPLES / 2U)) &&
               (denom_left > 0.0f) &&
               (denom_right > 0.0f)) {
                right_corr = numerator / sqrtf(denom_left * denom_right);
            } else {
                right_corr = best_corr;
            }
        } else {
            right_corr = best_corr;
        }

        // 阶段 3-6：对离散 lag 峰值做抛物线插值，细化 BPM 分辨率。
        analysis.autocorr_peak = best_corr;
        analysis.bpm_candidate =
            (60.0f * (float)PPG_SAMPLE_RATE_HZ) /
            ((float)best_lag + HeartRateAlgo_ParabolicRefineLag(left_corr, best_corr, right_corr));
        if((analysis.bpm_candidate >= (float)HR_MIN_BPM) &&
           (analysis.bpm_candidate <= (float)HR_MAX_BPM)) {
            analysis.valid = 1U;
        }
    }

    algo->latest_autocorr_peak = analysis.autocorr_peak;
    algo->latest_candidate_bpm = analysis.bpm_candidate;

    // 阶段 4：把 candidate_bpm 转成“可对外显示”的结果。
    if(analysis.valid == 0U) {
        algo->invalid_analysis_streak++;
        if(algo->invalid_analysis_streak >= HR_INVALID_TIMEOUT_WINDOWS) {
            algo->hr_valid = 0U;
            algo->state = HR_STATE_HOLD_INVALID;
        }
        return;
    }

    algo->invalid_analysis_streak = 0U;

    // 阶段 4-1：stable_bpm 是算法内部认定的稳定心率。
    if(algo->stable_bpm <= 0.0f) {
        algo->stable_bpm = analysis.bpm_candidate;
    } else if(fabsf(analysis.bpm_candidate - algo->stable_bpm) > HR_MAX_BPM_JUMP) {
        if((algo->pending_match_count == 0U) ||
           (fabsf(analysis.bpm_candidate - algo->pending_bpm) > 4.0f)) {
            algo->pending_bpm = analysis.bpm_candidate;
            algo->pending_match_count = 1U;
        } else {
            algo->pending_match_count++;
            if(algo->pending_match_count >= 2U) {
                algo->stable_bpm = analysis.bpm_candidate;
                algo->pending_match_count = 0U;
            }
        }
    } else {
        algo->stable_bpm = analysis.bpm_candidate;
        algo->pending_match_count = 0U;
    }

    // 阶段 4-2：display_bpm 再做一次缓慢跟随，减少页面抖动。
    if(algo->display_bpm <= 0.0f) {
        algo->display_bpm = algo->stable_bpm;
    } else {
        algo->display_bpm += 0.25f * (algo->stable_bpm - algo->display_bpm);
    }

    algo->hr_valid = 1U;
    algo->state = HR_STATE_TRACKING;
}

static void HeartRateAlgo_SetWarmupState(HeartRateAlgo_t * algo)
{
    if(algo == NULL) {
        return;
    }

    // 进入 WARMUP 只表示当前窗口还不能输出可信 BPM。
    algo->hr_valid = 0U;
    algo->latest_candidate_bpm = 0.0f;
    if(algo->state != HR_STATE_IDLE) {
        algo->state = HR_STATE_WARMUP;
    }
}

static void HeartRateAlgo_RecordTiming(HeartRateAlgo_t * algo,
                                       uint32_t sequence,
                                       uint32_t timestamp_ms)
{
    if(algo == NULL) {
        return;
    }

    // 运行时统计真实采样周期，后续 gap 判断依赖这个统计值。
    if(algo->have_last_timing != 0U) {
        if(timestamp_ms > algo->last_timestamp_ms) {
            HeartRateAlgo_UpdateExpectedInterval(algo, timestamp_ms - algo->last_timestamp_ms);
        }
    }

    algo->last_sequence = sequence;
    algo->last_timestamp_ms = timestamp_ms;
    algo->have_last_timing = 1U;
}

static uint32_t HeartRateAlgo_GetWindowIndex(const HeartRateAlgo_t * algo,
                                             uint32_t logical_index)
{
    uint32_t start;

    // logical_index 始终按“最旧 -> 最新”的顺序访问环形缓冲区。
    start = (algo->total_samples_in_window >= HR_WINDOW_SAMPLES) ?
        (uint32_t)algo->write_index : 0U;
    return (start + logical_index) % HR_WINDOW_SAMPLES;
}

static float HeartRateAlgo_GetGapThresholdMs(const HeartRateAlgo_t * algo)
{
    float expected_ms;

    // 尚未学习到真实采样周期前，先用 75ms 作为最小缺口阈值。
    if((algo == NULL) || (algo->expected_interval_ms_q8 == 0U)) {
        return (float)HR_TIMING_GAP_MIN_MS;
    }

    expected_ms = (float)algo->expected_interval_ms_q8 / 256.0f;
    if((expected_ms * 3.0f) > (float)HR_TIMING_GAP_MIN_MS) {
        return expected_ms * 3.0f;
    }

    return (float)HR_TIMING_GAP_MIN_MS;
}

static float HeartRateAlgo_ParabolicRefineLag(float left_corr,
                                              float center_corr,
                                              float right_corr)
{
    float denominator;

    // 用三点抛物线在离散峰值附近做亚样本插值。
    denominator = (left_corr - 2.0f * center_corr + right_corr);
    if(fabsf(denominator) < 0.0001f) {
        return 0.0f;
    }

    return 0.5f * (left_corr - right_corr) / denominator;
}
