#include "ppg_signal_processor.h"

#include <string.h>

/**
 * @file ppg_signal_processor.c
 * @brief PPG 前处理层。
 *
 * 这一层负责把原始传感器读数整理成：
 * raw_ppg -> baseline_ppg -> ac_ppg -> filtered_ppg
 *
 * 关键原则：
 * 1. 先判断样本能不能信，再决定是否更新内部滤波状态。
 * 2. 被拒绝样本不能污染 baseline_q8、smooth_buffer 和 last_filtered_ppg。
 * 3. 下游心率算法层直接消费 filtered_ppg，不在下游重复前处理。
 */

static void PpgSignalProcessor_FillCommonOutput(PpgProcessedSample_t * output,
                                                uint32_t sequence,
                                                uint32_t timestamp_ms,
                                                uint16_t raw_ppg,
                                                uint8_t valid,
                                                uint8_t flags);
static void PpgSignalProcessor_UpdateExpectedInterval(PpgSignalProcessor_t * processor,
                                                      uint32_t delta_ms);
static uint8_t PpgSignalProcessor_HasSequenceError(PpgSignalProcessor_t * processor,
                                                   uint32_t sequence);
static uint8_t PpgSignalProcessor_HasTimestampError(PpgSignalProcessor_t * processor,
                                                    uint32_t timestamp_ms);
static uint16_t PpgSignalProcessor_GetRawDeltaThreshold(const PpgSignalProcessor_t * processor);
static int32_t PpgSignalProcessor_GetAcRejectThreshold(const PpgSignalProcessor_t * processor);
static int32_t PpgSignalProcessor_GetCurrentBaseline(const PpgSignalProcessor_t * processor);

void PpgSignalProcessor_Init(PpgSignalProcessor_t * processor)
{
    if(processor == NULL) {
        return;
    }

    memset(processor, 0, sizeof(*processor));
    processor->initialized = 1U;
}

uint8_t PpgSignalProcessor_IsGrossOutlier(const PpgSignalProcessor_t * processor,
                                          uint16_t raw_ppg)
{
    uint16_t delta;
    uint16_t threshold;

    // 阶段 0：只有拿到过上一笔可信 raw，才有资格做“瞬时跳变过大”判断。
    if((processor == NULL) || (processor->previous_valid_raw_valid == 0U)) {
        return 0U;
    }

    if(raw_ppg >= processor->previous_valid_raw) {
        delta = (uint16_t)(raw_ppg - processor->previous_valid_raw);
    } else {
        delta = (uint16_t)(processor->previous_valid_raw - raw_ppg);
    }

    // 阶段 0：阈值随当前基线动态变化，而不是完全写死。
    threshold = PpgSignalProcessor_GetRawDeltaThreshold(processor);
    return (delta > threshold) ? 1U : 0U;
}

void PpgSignalProcessor_ProcessValidSample(PpgSignalProcessor_t * processor,
                                           uint32_t sequence,
                                           uint32_t timestamp_ms,
                                           uint16_t raw_ppg,
                                           uint8_t flags,
                                           PpgProcessedSample_t * output)
{
    int32_t baseline_ppg;
    int32_t ac_ppg;
    int32_t filtered_ppg;
    int32_t target_q8;
    uint8_t sample_valid = 1U;

    if((processor == NULL) || (output == NULL)) {
        return;
    }

    if(processor->initialized == 0U) {
        PpgSignalProcessor_Init(processor);
    }

    // 阶段 1：先基于“上一时刻已经确认有效”的状态，估算本样本的 baseline 和 AC。
    // 注意这一步只是为拒绝判断提供参考，尚未真正推进滤波状态。
    baseline_ppg = PpgSignalProcessor_GetCurrentBaseline(processor);
    ac_ppg = (int32_t)raw_ppg - baseline_ppg;
    filtered_ppg = processor->last_filtered_ppg;

    // 第一笔可信样本到来前，baseline 尚未建立。
    // 此时不能用 "raw - 0" 的巨大交流分量去做异常拒绝，否则会把所有首包都误判为异常。
    if(processor->valid_sample_count == 0U) {
        baseline_ppg = (int32_t)raw_ppg;
        ac_ppg = 0;
    }

    // 阶段 2：前级硬拒绝。
    // 这里只决定样本能不能进入状态机，不更新 baseline 与平滑滤波。
    if(raw_ppg == 0U) {
        sample_valid = 0U;
        flags |= PPG_SAMPLE_FLAG_GROSS_OUTLIER;
    }

    if(PpgSignalProcessor_HasSequenceError(processor, sequence) != 0U) {
        sample_valid = 0U;
        flags |= PPG_SAMPLE_FLAG_SEQUENCE_ERROR;
        processor->flag_reject_count++;
    }

    if(PpgSignalProcessor_HasTimestampError(processor, timestamp_ms) != 0U) {
        sample_valid = 0U;
        flags |= PPG_SAMPLE_FLAG_TIMESTAMP_ERROR;
        processor->flag_reject_count++;
    }

    // 阶段 2-1：对比上一笔可信 raw，拒绝突发毛刺。
    if((sample_valid != 0U) && (processor->previous_valid_raw_valid != 0U)) {
        uint16_t delta;
        uint16_t threshold;

        if(raw_ppg >= processor->previous_valid_raw) {
            delta = (uint16_t)(raw_ppg - processor->previous_valid_raw);
        } else {
            delta = (uint16_t)(processor->previous_valid_raw - raw_ppg);
        }

        threshold = PpgSignalProcessor_GetRawDeltaThreshold(processor);
        if(delta > threshold) {
            sample_valid = 0U;
            flags |= PPG_SAMPLE_FLAG_GROSS_OUTLIER;
        }
    }

    // 阶段 2-2：AC 分量若远超正常脉搏波范围，也拒绝该样本。
    if((sample_valid != 0U) &&
       (processor->valid_sample_count != 0U) &&
       ((ac_ppg > PpgSignalProcessor_GetAcRejectThreshold(processor)) ||
        (ac_ppg < -PpgSignalProcessor_GetAcRejectThreshold(processor)))) {
        sample_valid = 0U;
        flags |= PPG_SAMPLE_FLAG_GROSS_OUTLIER;
    }

    // 阶段 2-3：拒绝样本时，只输出诊断字段，不得污染前处理状态。
    if(sample_valid == 0U) {
        processor->invalid_sample_count++;
        if((flags & PPG_SAMPLE_FLAG_GROSS_OUTLIER) != 0U) {
            processor->gross_outlier_count++;
            processor->raw_reject_count++;
        }

        PpgSignalProcessor_FillCommonOutput(output,
                                            sequence,
                                            timestamp_ms,
                                            raw_ppg,
                                            0U,
                                            flags);
        output->baseline_ppg = baseline_ppg;
        output->ac_ppg = ac_ppg;
        output->filtered_ppg = processor->last_filtered_ppg;

        processor->previous_sequence = sequence;
        processor->previous_timestamp_ms = timestamp_ms;
        processor->previous_sample_seen = 1U;
        return;
    }

    // 阶段 3：样本通过门控后，才允许真正更新 baseline。
    if(processor->valid_sample_count == 0U) {
        processor->baseline_q8 = ((int32_t)raw_ppg << 8U);
    } else {
        target_q8 = ((int32_t)raw_ppg << 8U);
        processor->baseline_q8 +=
            (target_q8 - processor->baseline_q8) >> PPG_DC_BASELINE_SHIFT;
    }

    baseline_ppg = processor->baseline_q8 >> 8U;
    ac_ppg = (int32_t)raw_ppg - baseline_ppg;

    // 阶段 4：把 ac_ppg 放入平滑窗口，生成 filtered_ppg。
    // 这里使用轻量滑动平均，目的是给下游自相关提供更稳的波形。
    if(processor->smooth_count < PPG_SMOOTH_WINDOW_SIZE) {
        processor->smooth_buffer[processor->smooth_index] = ac_ppg;
        processor->smooth_sum += ac_ppg;
        processor->smooth_count++;
    } else {
        processor->smooth_sum -= processor->smooth_buffer[processor->smooth_index];
        processor->smooth_buffer[processor->smooth_index] = ac_ppg;
        processor->smooth_sum += ac_ppg;
    }

    processor->smooth_index++;
    if(processor->smooth_index >= PPG_SMOOTH_WINDOW_SIZE) {
        processor->smooth_index = 0U;
    }

    filtered_ppg = processor->smooth_sum / (int32_t)processor->smooth_count;

    // 阶段 5：更新统计量与状态位。
    processor->valid_sample_count++;
    if(processor->valid_sample_count <= PPG_WARMUP_SAMPLE_COUNT) {
        processor->warmup_sample_count++;
        flags |= PPG_SAMPLE_FLAG_WARMUP;
    }

    if(processor->smooth_count >= PPG_SMOOTH_WINDOW_SIZE) {
        processor->filter_ready = 1U;
        flags |= PPG_SAMPLE_FLAG_FILTER_READY;
    } else {
        processor->filter_ready = 0U;
    }

    if((flags & PPG_SAMPLE_FLAG_RETRY_SUCCESS) != 0U) {
        processor->retry_success_count++;
    }

    processor->previous_valid_raw = raw_ppg;
    processor->previous_valid_raw_valid = 1U;
    processor->previous_sequence = sequence;
    processor->previous_timestamp_ms = timestamp_ms;
    processor->previous_sample_seen = 1U;
    processor->last_baseline_ppg = baseline_ppg;
    processor->last_filtered_ppg = filtered_ppg;

    // 阶段 6：组装输出样本，交给心率算法层和串口 CSV。
    PpgSignalProcessor_FillCommonOutput(output,
                                        sequence,
                                        timestamp_ms,
                                        raw_ppg,
                                        1U,
                                        flags);
    output->baseline_ppg = baseline_ppg;
    output->ac_ppg = ac_ppg;
    output->filtered_ppg = filtered_ppg;
}

void PpgSignalProcessor_ProcessInvalidSample(PpgSignalProcessor_t * processor,
                                             uint32_t sequence,
                                             uint32_t timestamp_ms,
                                             uint8_t flags,
                                             PpgProcessedSample_t * output)
{
    int32_t baseline_ppg = 0;

    if(output == NULL) {
        return;
    }

    if(processor != NULL) {
        if(processor->initialized == 0U) {
            PpgSignalProcessor_Init(processor);
        }

        // 无效样本也保留当前 baseline 观测值，方便串口回放前后文；
        // 但它不会推进 baseline 和滤波器状态。
        baseline_ppg = PpgSignalProcessor_GetCurrentBaseline(processor);
        processor->invalid_sample_count++;
        if((flags & PPG_SAMPLE_FLAG_I2C_ERROR) != 0U) {
            processor->flag_reject_count++;
        }
        if(PpgSignalProcessor_HasSequenceError(processor, sequence) != 0U) {
            flags |= PPG_SAMPLE_FLAG_SEQUENCE_ERROR;
            processor->flag_reject_count++;
        }
        if(PpgSignalProcessor_HasTimestampError(processor, timestamp_ms) != 0U) {
            flags |= PPG_SAMPLE_FLAG_TIMESTAMP_ERROR;
            processor->flag_reject_count++;
        }

        processor->previous_sequence = sequence;
        processor->previous_timestamp_ms = timestamp_ms;
        processor->previous_sample_seen = 1U;
    }

    PpgSignalProcessor_FillCommonOutput(output,
                                        sequence,
                                        timestamp_ms,
                                        0U,
                                        0U,
                                        flags);
    output->baseline_ppg = baseline_ppg;
    output->ac_ppg = 0;
    output->filtered_ppg = (processor != NULL) ? processor->last_filtered_ppg : 0;
}

static void PpgSignalProcessor_FillCommonOutput(PpgProcessedSample_t * output,
                                                uint32_t sequence,
                                                uint32_t timestamp_ms,
                                                uint16_t raw_ppg,
                                                uint8_t valid,
                                                uint8_t flags)
{
    // 统一维护 CSV 公共字段，保证串口输出字段顺序固定。
    output->sequence = sequence;
    output->timestamp_ms = timestamp_ms;
    output->raw_ppg = raw_ppg;
    output->valid = valid;
    output->flags = flags;
}

static void PpgSignalProcessor_UpdateExpectedInterval(PpgSignalProcessor_t * processor,
                                                      uint32_t delta_ms)
{
    uint32_t target_q8;

    // 用 Q8 定点方式平滑估计采样周期，避免完全硬编码 25ms。
    target_q8 = delta_ms << 8U;
    if(processor->expected_interval_ms_q8 == 0U) {
        processor->expected_interval_ms_q8 = target_q8;
    } else {
        processor->expected_interval_ms_q8 =
            (processor->expected_interval_ms_q8 * 7U + target_q8) / 8U;
    }
}

static uint8_t PpgSignalProcessor_HasSequenceError(PpgSignalProcessor_t * processor,
                                                   uint32_t sequence)
{
    uint32_t lost_samples = 0U;

    if((processor == NULL) || (processor->previous_sample_seen == 0U)) {
        return 0U;
    }

    // 正常情况：sequence 必须严格递增 1。
    if(sequence == (processor->previous_sequence + 1U)) {
        return 0U;
    }

    // 重复、倒退、跳号都记为 sequence 异常。
    processor->sequence_gap_count++;
    if(sequence > processor->previous_sequence) {
        lost_samples = sequence - processor->previous_sequence - 1U;
        processor->lost_sample_count += lost_samples;
    }

    return 1U;
}

static uint8_t PpgSignalProcessor_HasTimestampError(PpgSignalProcessor_t * processor,
                                                    uint32_t timestamp_ms)
{
    uint32_t delta_ms;
    uint32_t dynamic_threshold_ms;

    if((processor == NULL) || (processor->previous_sample_seen == 0U)) {
        return 0U;
    }

    // 时间戳不递增时，当前样本直接视为时序异常。
    if(timestamp_ms <= processor->previous_timestamp_ms) {
        processor->timestamp_gap_count++;
        return 1U;
    }

    delta_ms = timestamp_ms - processor->previous_timestamp_ms;
    dynamic_threshold_ms = PPG_MAX_TIMESTAMP_GAP_MS;
    if(processor->expected_interval_ms_q8 != 0U) {
        uint32_t expected_ms = processor->expected_interval_ms_q8 >> 8U;
        uint32_t scaled_ms = expected_ms * 3U;
        if(scaled_ms > dynamic_threshold_ms) {
            dynamic_threshold_ms = scaled_ms;
        }
    }

    // 采样间隔明显超出运行时统计值，视为发生采样缺口。
    if(delta_ms > dynamic_threshold_ms) {
        processor->timestamp_gap_count++;
        return 1U;
    }

    PpgSignalProcessor_UpdateExpectedInterval(processor, delta_ms);
    return 0U;
}

static uint16_t PpgSignalProcessor_GetRawDeltaThreshold(const PpgSignalProcessor_t * processor)
{
    uint32_t baseline_ppg;
    uint32_t threshold;

    if(processor == NULL) {
        return PPG_RAW_DELTA_ABS_FLOOR;
    }

    // baseline 越高，允许的 raw 跳变阈值可略微放宽，但保留绝对下限。
    baseline_ppg = (uint32_t)PpgSignalProcessor_GetCurrentBaseline(processor);
    threshold = baseline_ppg / PPG_RAW_DELTA_BASELINE_DIV;
    if(threshold < PPG_RAW_DELTA_ABS_FLOOR) {
        threshold = PPG_RAW_DELTA_ABS_FLOOR;
    }

    return (uint16_t)threshold;
}

static int32_t PpgSignalProcessor_GetAcRejectThreshold(const PpgSignalProcessor_t * processor)
{
    uint32_t baseline_ppg;
    uint32_t threshold;

    if(processor == NULL) {
        return (int32_t)PPG_AC_ABS_FLOOR;
    }

    // AC 拒绝阈值同样根据 baseline 自适应，适配不同接触强度。
    baseline_ppg = (uint32_t)PpgSignalProcessor_GetCurrentBaseline(processor);
    threshold = baseline_ppg / PPG_AC_BASELINE_DIV;
    if(threshold < PPG_AC_ABS_FLOOR) {
        threshold = PPG_AC_ABS_FLOOR;
    }

    return (int32_t)threshold;
}

static int32_t PpgSignalProcessor_GetCurrentBaseline(const PpgSignalProcessor_t * processor)
{
    if((processor == NULL) || (processor->valid_sample_count == 0U)) {
        return 0;
    }

    return processor->baseline_q8 >> 8U;
}
