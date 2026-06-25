#include "ppg_signal_processor.h"

#include <string.h>

/**
 * @brief 填写输出样本的公共字段。
 */
static void PpgSignalProcessor_FillCommonOutput(PpgProcessedSample_t *output,
                                                uint32_t sequence,
                                                uint32_t timestamp_ms,
                                                uint16_t raw_ppg,
                                                uint8_t valid,
                                                uint8_t flags)
{
    output->sequence = sequence;
    output->timestamp_ms = timestamp_ms;
    output->raw_ppg = raw_ppg;
    output->valid = valid;
    output->flags = flags;
}

void PpgSignalProcessor_Init(PpgSignalProcessor_t *processor)
{
    if(processor == NULL) {
        return;
    }

    memset(processor, 0, sizeof(*processor));
    processor->initialized = 1U;
}

uint8_t PpgSignalProcessor_IsGrossOutlier(const PpgSignalProcessor_t *processor,
                                          uint16_t raw_ppg)
{
    uint16_t previous_raw;
    uint16_t delta;

    if((processor == NULL) || (processor->previous_valid_raw_valid == 0U)) {
        return 0U;
    }

    previous_raw = processor->previous_valid_raw;
    if(raw_ppg >= previous_raw) {
        delta = (uint16_t)(raw_ppg - previous_raw);
    } else {
        delta = (uint16_t)(previous_raw - raw_ppg);
    }

    return (delta > PPG_GROSS_JUMP_THRESHOLD) ? 1U : 0U;
}

void PpgSignalProcessor_ProcessValidSample(PpgSignalProcessor_t *processor,
                                           uint32_t sequence,
                                           uint32_t timestamp_ms,
                                           uint16_t raw_ppg,
                                           uint8_t flags,
                                           PpgProcessedSample_t *output)
{
    int32_t target_q8;
    int32_t baseline_ppg;
    int32_t ac_ppg;
    int32_t filtered_ppg;
    uint8_t window_length;

    if((processor == NULL) || (output == NULL)) {
        return;
    }

    if(processor->initialized == 0U) {
        PpgSignalProcessor_Init(processor);
    }

    if(processor->valid_sample_count == 0U) {
        processor->baseline_q8 = ((int32_t)raw_ppg << 8U);
    } else {
        target_q8 = ((int32_t)raw_ppg << 8U);
        processor->baseline_q8 +=
            (target_q8 - processor->baseline_q8) >> PPG_DC_BASELINE_SHIFT;
    }

    baseline_ppg = processor->baseline_q8 >> 8U;
    ac_ppg = (int32_t)raw_ppg - baseline_ppg;

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

    window_length = processor->smooth_count;
    filtered_ppg = processor->smooth_sum / (int32_t)window_length;

    processor->valid_sample_count++;
    if(processor->valid_sample_count <= PPG_WARMUP_SAMPLE_COUNT) {
        flags |= PPG_SAMPLE_FLAG_WARMUP;
        processor->warmup_sample_count++;
    }

    if((processor->smooth_count >= PPG_SMOOTH_WINDOW_SIZE) &&
       (processor->valid_sample_count > PPG_WARMUP_SAMPLE_COUNT)) {
        processor->filter_ready = 1U;
        flags |= PPG_SAMPLE_FLAG_FILTER_READY;
    } else {
        processor->filter_ready = 0U;
    }

    processor->previous_valid_raw = raw_ppg;
    processor->previous_valid_raw_valid = 1U;

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

void PpgSignalProcessor_ProcessInvalidSample(PpgSignalProcessor_t *processor,
                                             uint32_t sequence,
                                             uint32_t timestamp_ms,
                                             uint8_t flags,
                                             PpgProcessedSample_t *output)
{
    if(output == NULL) {
        return;
    }

    if(processor != NULL) {
        if(processor->initialized == 0U) {
            PpgSignalProcessor_Init(processor);
        }
        processor->invalid_sample_count++;
    }

    PpgSignalProcessor_FillCommonOutput(output,
                                        sequence,
                                        timestamp_ms,
                                        0U,
                                        0U,
                                        flags);
    output->baseline_ppg = 0;
    output->ac_ppg = 0;
    output->filtered_ppg = 0;
}
