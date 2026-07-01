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
 * 这一层的职责可以概括为：
 * 1. 维护 filtered_ppg 的环形滑动窗口；
 * 2. 记录每一拍是否有效，避免坏点直接参与相关计算；
 * 3. 周期性对整窗数据做自相关分析，求出候选心率；
 * 4. 对候选心率做跳变抑制和显示平滑，得到最终对外的 display_bpm。
 */

typedef struct
{
    float bpm_candidate;
    float autocorr_peak;
    float valid_ratio;
    float signal_std;
    uint8_t valid;
} HeartRateAlgoAnalysis_t;

/**
 * @brief 向环形窗口写入一个心率算法输入点。
 */
static void HeartRateAlgo_PushWindowSample(HeartRateAlgo_t * algo,
                                           int32_t filtered_ppg,
                                           uint8_t valid);
/**
 * @brief 对单个 filtered_ppg 做 3 点中值滤波，进一步压制孤立尖峰。
 */
static int32_t HeartRateAlgo_ApplyMedian3(HeartRateAlgo_t * algo,
                                          int32_t filtered_ppg);
/**
 * @brief 对当前整窗数据执行一次心率分析。
 */
static void HeartRateAlgo_RunAnalysis(HeartRateAlgo_t * algo);
/**
 * @brief 把算法状态切回预热态，并清理当前输出有效性。
 */
static void HeartRateAlgo_SetWarmupState(HeartRateAlgo_t * algo);
/**
 * @brief 记录当前样本的时序信息。
 */
static void HeartRateAlgo_RecordTiming(HeartRateAlgo_t * algo,
                                       uint32_t sequence,
                                       uint32_t timestamp_ms);
/**
 * @brief 按“最旧到最新”的逻辑顺序访问环形窗口元素。
 */
static uint32_t HeartRateAlgo_GetWindowIndex(const HeartRateAlgo_t * algo,
                                             uint32_t logical_index);
/**
 * @brief 对离散自相关峰值做三点抛物线插值。
 */
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
    uint8_t sample_valid;
    uint8_t hr_input_valid;
    uint8_t filter_ready;
    uint8_t has_sequence_error;
    uint8_t has_timestamp_error;

    if((algo == NULL) || (sample == NULL)) {
        return;
    }

    if(algo->initialized == 0U) {
        HeartRateAlgo_Init(algo);
    }

    // 把“总线是否正常”“前处理是否认可”“能否进入心率窗口”分开记录，
    // 这样调试时可以区分问题出在采样链路、前处理，还是心率分析自身的门控条件。
    algo->latest_bus_ok = ((sample->flags & PPG_SAMPLE_FLAG_I2C_ERROR) == 0U) ? 1U : 0U;
    sample_valid = sample->valid;//  *取前处理判定的数据有效性
    algo->latest_sample_valid = sample_valid;
    algo->latest_hr_input_valid = 0U;//  *数据进入心率窗口的标志， 先清零

    if(algo->state == HR_STATE_IDLE) {
        algo->state = HR_STATE_WARMUP;
    }

    // 前处理层已经完成样本级时序判定。
    // 这里不再重复重算 sequence / timestamp 是否异常，只根据错误标志决定整窗是否作废。
    has_sequence_error = ((sample->flags & PPG_SAMPLE_FLAG_SEQUENCE_ERROR) != 0U) ? 1U : 0U;
    has_timestamp_error = ((sample->flags & PPG_SAMPLE_FLAG_TIMESTAMP_ERROR) != 0U) ? 1U : 0U;
    if((has_sequence_error != 0U) || (has_timestamp_error != 0U)) {
        if(has_sequence_error != 0U) {
            algo->sequence_gap_count++;
            if((algo->have_last_timing != 0U) && (sample->sequence > (algo->last_sequence + 1U))) {
                algo->lost_sample_count += sample->sequence - algo->last_sequence - 1U;
            }
        }

        if(has_timestamp_error != 0U) {
            algo->timestamp_gap_count++;
        }

        HeartRateAlgo_Reset(algo, HR_STATE_WARMUP);
        HeartRateAlgo_RecordTiming(algo, sample->sequence, sample->timestamp_ms);
        return;
    }

    // 前处理层虽然已经给出 filtered_ppg，但心率算法仍要再做一次“能否入窗”的业务判定。
    // 只有滤波窗口准备好且样本没有关键错误标志时，这一拍才允许参与周期估计。
    filter_ready = ((sample->flags & PPG_SAMPLE_FLAG_FILTER_READY) != 0U) ? 1U : 0U;//  *取滤波窗口就绪标志
    hr_input_valid = sample_valid;//  *前处理判定的数据有效性
    if((filter_ready == 0U) ||
       ((sample->flags & (PPG_SAMPLE_FLAG_I2C_ERROR |
                          PPG_SAMPLE_FLAG_GROSS_OUTLIER |
                          PPG_SAMPLE_FLAG_SEQUENCE_ERROR |
                          PPG_SAMPLE_FLAG_TIMESTAMP_ERROR)) != 0U)) {
        hr_input_valid = 0U;
    }

    // 有效点先过一层 median3，目的是再压掉偶发单点尖刺，同时尽量不破坏真实脉搏周期。
    if(hr_input_valid != 0U) {
        HeartRateAlgo_PushWindowSample(algo,
                                       HeartRateAlgo_ApplyMedian3(algo, sample->filtered_ppg),
                                       1U);//  *有效点放入心率算法窗口
    } else {
        // 无效点仍然占据窗口里的一个时间槽位，只是 valid_mask 标记为 0，
        // 这样后续相关计算仍能感知“这一拍发生过，但不可用”。
        HeartRateAlgo_PushWindowSample(algo, 0, 0U);
    }

    algo->latest_hr_input_valid = hr_input_valid;
    HeartRateAlgo_RecordTiming(algo, sample->sequence, sample->timestamp_ms);

    // 自相关分析计算量比单点入窗大得多，不需要每拍都跑；
    // 固定每若干拍分析一次，可以在响应速度和 MCU 开销之间取平衡。
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

static void HeartRateAlgo_PushWindowSample(HeartRateAlgo_t * algo,
                                           int32_t filtered_ppg,
                                           uint8_t valid)
{
    uint32_t index;

    if(algo == NULL) {
        return;
    }

    // 环形滑窗：write_index 始终指向“下一笔待写入位置”。
    // 当窗口已满时，先把最旧位置原来的 valid 贡献扣掉，再覆写成当前样本。
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
    // 它不追求强滤波，只是把“孤立单点”对自相关峰值的干扰再压低一层。
    if(algo == NULL) {
        return filtered_ppg;
    }

    //  *往环形缓冲区写入当前的 AC分量
    algo->median_history[algo->median_write_index] = filtered_ppg;
    algo->median_write_index++;
    if(algo->median_write_index >= HR_MEDIAN_WINDOW_SIZE) {
        algo->median_write_index = 0U;
    }

    //  *缓冲区没满的时候， 直接返回当前的AC 分量
    if(algo->median_count < HR_MEDIAN_WINDOW_SIZE) {
        algo->median_count++;
        return filtered_ppg;
    }

    // *缓冲区满了  将a,b,c从小到大排序
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

    //  *返回中间值
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

    //  *看 心率窗口（缓冲区）的数据 填满没有
    if(algo->total_samples_in_window < HR_WINDOW_SAMPLES) {
        HeartRateAlgo_SetWarmupState(algo);
        return;
    }

    // *判断有效点数 够不够
    analysis.valid_ratio = (float)algo->valid_samples_in_window / (float)HR_WINDOW_SAMPLES;//有效点数占一个窗口数据的比率
    algo->latest_window_valid_ratio = analysis.valid_ratio;//把当前的比率 拿来 更新上一次的 有效比率
    valid_required = (uint32_t)(HR_WINDOW_SAMPLES * HR_MIN_VALID_RATIO);//计算要求的 有效点个数
    if((analysis.valid_ratio < HR_MIN_VALID_RATIO) ||
       (algo->valid_samples_in_window < valid_required)) {
        HeartRateAlgo_SetWarmupState(algo);// 如果 比率不够 或者 有效点数不够
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

        // 先算均值，把整窗波形平移到以 0 为中心。
        // 这样做可以把自相关更聚焦在周期起伏本身，而不是直流偏置上。
        for(i = 0U; i < HR_WINDOW_SAMPLES; i++) {
            uint32_t idx = HeartRateAlgo_GetWindowIndex(algo, i);//  *找到这一个窗口 第i个数据 在缓冲区真实的存储索引
            if(algo->sample_valid_mask[idx] != 0U) {
                mean += (float)algo->filtered_signal[idx];//  *如果这个数据有效，就把它的值加到总和
                valid_count++;//  *记录有效数据个数
            }
        }

        if(valid_count < valid_required) {
            HeartRateAlgo_SetWarmupState(algo);
            return;//  *如果有效数据个数不够， 先不算心率， 等后续新样本进入窗口再判断
        }

        mean /= (float)valid_count;//计算均值

        // 再看信号起伏够不够大。
        //  *下面计算 有效点 的 标准差
        for(i = 0U; i < HR_WINDOW_SAMPLES; i++) {
            uint32_t idx = HeartRateAlgo_GetWindowIndex(algo, i);
            float centered;

            if(algo->sample_valid_mask[idx] == 0U) {//  跳过无效点
                continue;
            }

            centered = (float)algo->filtered_signal[idx] - mean;//计算样本点和均值的差
            variance_sum += centered * centered;//计算
        }

        analysis.signal_std = sqrtf(variance_sum / (float)valid_count);
        //  *如果标准差太小， 意味着起伏太小， 不适合计算心率
        if(analysis.signal_std < HR_MIN_SIGNAL_STD) {
            HeartRateAlgo_SetWarmupState(algo);
            return;
        }

        //  *自相关算法，找心率信号波形的周期，有了周期就可以算出心率了
        // 在允许的心率范围内搜索自相关主峰。 就是找最有可能的周期 是几个点 i从13到53，一个个找
        //  *i从13 - 53 是因为预设了一个 合理的 心率 从大约 184 - 45
        //主峰就是随着 i 变化， 会有一些列的 相关系数， 主峰就是相关系数最接近1 的那里
        // lag 越小，对应 BPM 越高；lag 越大，对应 BPM 越低。
        for(i = HR_MIN_LAG_SAMPLES; i <= HR_MAX_LAG_SAMPLES; i++) {
            uint32_t pair_count = 0U;
            uint32_t j;
            float numerator = 0.0f;
            float denom_left = 0.0f;
            float denom_right = 0.0f;
            float corr;

            //  *按照当前周期 i，在两个周期的相同位置找两个点，计算自相关 系数
            for(j = i; j < HR_WINDOW_SAMPLES; j++) {//  *从 i开始 是为了让 j-i>=0, 防止找到的上一个点的索引不存在
                uint32_t idx_now = HeartRateAlgo_GetWindowIndex(algo, j);
                uint32_t idx_prev = HeartRateAlgo_GetWindowIndex(algo, j - i);
                float left;
                float right;

                if((algo->sample_valid_mask[idx_now] == 0U) ||
                   (algo->sample_valid_mask[idx_prev] == 0U)) {
                    continue;//  *如果找到的点是无效点，跳过
                }

                left = (float)algo->filtered_signal[idx_now] - mean;
                right = (float)algo->filtered_signal[idx_prev] - mean;
                numerator += left * right;//计算分子
                denom_left += left * left;//计算分母的其中一个因子
                denom_right += right * right;//另一个因子
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

        //  *判断周期 在 合适范围， 判断主峰 有没有超过阈值 HR_MIN_AUTOCORR_PEAK 0.55， 超过阈值的更接近正确的周期
        if((best_lag < HR_MIN_LAG_SAMPLES) ||
           (best_lag > HR_MAX_LAG_SAMPLES) ||
           (best_corr < HR_MIN_AUTOCORR_PEAK)) {
            HeartRateAlgo_SetWarmupState(algo);
            algo->latest_autocorr_peak = (best_corr > 0.0f) ? best_corr : 0.0f;
            return;
        }

        // 额外计算主峰左右各一个相邻 lag 的相关值，
        // 后面会用它们对离散峰值做亚样本插值，提高 BPM 分辨率。
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

    // 候选 BPM 无效时，不立即清空显示值，而是统计连续失败次数。
    // 这样可以避免偶发一窗不可靠就让页面上的心率数字瞬间归零。
    if(analysis.valid == 0U) {
        algo->invalid_analysis_streak++;
        if(algo->invalid_analysis_streak >= HR_INVALID_TIMEOUT_WINDOWS) {
            algo->hr_valid = 0U;
            algo->state = HR_STATE_HOLD_INVALID;
        }
        return;
    }

    algo->invalid_analysis_streak = 0U;

    // stable_bpm 表示算法内部目前接受的稳定心率。
    // 如果新候选值和稳定值差太大，不立即跳过去，而是要求连续两次接近的新候选再切换。
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

    // display_bpm 是页面最终看到的数值。
    // 它在 stable_bpm 基础上再做一次缓跟随，避免用户看到每窗都发生明显跳字。
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
    // 这里不会清空 display_bpm，本意是把“当前窗无效”和“页面立刻清零”区分开。
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
