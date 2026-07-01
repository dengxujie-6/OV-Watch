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

/**
 * @brief 填充所有样本都会携带的公共输出字段。
 *
 * @param output 输出样本对象，不允许为 NULL。
 * @param sequence 当前采样序号。
 * @param timestamp_ms 当前采样时间戳，单位 ms。
 * @param raw_ppg 当前原始 PPG 读数。
 * @param valid 当前样本是否有效。
 * @param flags 当前样本附加标志位集合。
 */
static void PpgSignalProcessor_FillCommonOutput(PpgProcessedSample_t * output,
                                                uint32_t sequence,
                                                uint32_t timestamp_ms,
                                                uint16_t raw_ppg,
                                                uint8_t valid,
                                                uint8_t flags);
/**
 * @brief 根据最近一次有效采样间隔，更新运行时期望采样周期。
 *
 * @param processor 前处理器实例，不允许为 NULL。
 * @param delta_ms 当前样本与上一拍之间的时间差，单位 ms。
 */
static void PpgSignalProcessor_UpdateExpectedInterval(PpgSignalProcessor_t * processor,
                                                      uint32_t delta_ms);
/**
 * @brief 检查采样序号是否发生重复、倒退或跳号。
 *
 * @param processor 前处理器实例，不允许为 NULL。
 * @param sequence 当前采样序号。
 *
 * @return 1 表示序号异常；0 表示序号连续。
 */
static uint8_t PpgSignalProcessor_HasSequenceError(PpgSignalProcessor_t * processor,
                                                   uint32_t sequence);
/**
 * @brief 检查当前时间戳是否满足递增且间隔合理。
 *
 * @param processor 前处理器实例，不允许为 NULL。
 * @param timestamp_ms 当前样本时间戳，单位 ms。
 *
 * @return 1 表示时间戳异常；0 表示时间戳可接受。
 */
static uint8_t PpgSignalProcessor_HasTimestampError(PpgSignalProcessor_t * processor,
                                                    uint32_t timestamp_ms);
/**
 * @brief 结合当前基线幅值，计算 raw_ppg 的动态跳变阈值。
 *
 * @param processor 前处理器实例，允许为 NULL。
 *
 * @return 当前样本允许的原始幅值跳变上限。
 */
static uint16_t PpgSignalProcessor_GetRawDeltaThreshold(const PpgSignalProcessor_t * processor);
/**
 * @brief 结合当前基线幅值，计算 ac_ppg 的动态拒绝阈值。
 *
 * @param processor 前处理器实例，允许为 NULL。
 *
 * @return 当前样本允许的交流分量绝对值上限。
 */
static int32_t PpgSignalProcessor_GetAcRejectThreshold(const PpgSignalProcessor_t * processor);
/**
 * @brief 读取当前已经建立好的直流基线值。
 *
 * @param processor 前处理器实例，允许为 NULL。
 *
 * @return 当前基线；如果尚未建立有效基线，则返回 0。
 */
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
    // *注意这一步只是为拒绝判断提供参考，尚未真正推进滤波状态。
    baseline_ppg = PpgSignalProcessor_GetCurrentBaseline(processor);
    ac_ppg = (int32_t)raw_ppg - baseline_ppg;//当前信号，减去基线，相当于交流分量
    filtered_ppg = processor->last_filtered_ppg;

    // 第一笔可信样本到来前，baseline 尚未建立。
    // 此时不能用 "raw - 0" 的巨大交流分量去做异常拒绝，否则会把所有首包都误判为异常。
    if(processor->valid_sample_count == 0U) {
        baseline_ppg = (int32_t)raw_ppg;
        ac_ppg = 0;
    }

    // * 检查 raw_ppg 是正常测到的数据， 判断数据连续性， 采样周期合理性
    // 这里只决定样本能不能进入状态机，不更新 baseline 与平滑滤波。
    if(raw_ppg == 0U) {
        // ! PPG 原始值为 0 通常代表总线错误、器件未工作或无意义占位值，直接拒绝最稳妥。
        sample_valid = 0U;
        flags |= PPG_SAMPLE_FLAG_GROSS_OUTLIER;
    }

    if(PpgSignalProcessor_HasSequenceError(processor, sequence) != 0U) {//  *严格的顺序要求，保证数据是一个个连续的
        sample_valid = 0U;
        flags |= PPG_SAMPLE_FLAG_SEQUENCE_ERROR;
        processor->flag_reject_count++;
    }

    if(PpgSignalProcessor_HasTimestampError(processor, timestamp_ms) != 0U) {//  *和动态的采样间隔比较，提高系统鲁棒性
        sample_valid = 0U;
        flags |= PPG_SAMPLE_FLAG_TIMESTAMP_ERROR;
        processor->flag_reject_count++;
    }

    //  *检查值的大小是否合理， 毛刺检查， AC分量检查
    // 阶段 2-1：对比上一笔可信 raw，拒绝突发毛刺。
    if((sample_valid != 0U) && (processor->previous_valid_raw_valid != 0U)) {
        uint16_t delta;
        uint16_t threshold;

        //  *获得当前有效测量值和上一个有效测量值的差 绝对值
        if(raw_ppg >= processor->previous_valid_raw) {
            delta = (uint16_t)(raw_ppg - processor->previous_valid_raw);
        } else {
            delta = (uint16_t)(processor->previous_valid_raw - raw_ppg);
        }

        threshold = PpgSignalProcessor_GetRawDeltaThreshold(processor);//门槛设置为当前基线 ± 10%， threshold也是一个绝对值
        if(delta > threshold) {
            // ! 这里只和“上一笔可信样本”比较，避免把已经判坏的数据继续当作参考基准。
            sample_valid = 0U;
            flags |= PPG_SAMPLE_FLAG_GROSS_OUTLIER;
        }
    }

    // 阶段 2-2：AC 分量若远超正常脉搏波范围，也拒绝该样本。
    if((sample_valid != 0U) &&
       (processor->valid_sample_count != 0U) &&
       ((ac_ppg > PpgSignalProcessor_GetAcRejectThreshold(processor)) ||//门槛设置为当前基线的 ± 1/12
        (ac_ppg < -PpgSignalProcessor_GetAcRejectThreshold(processor)))) {
        // ! 这里使用对称阈值，意味着过高和过低的异常摆动都会破坏后续自相关窗口。
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

        // * 虽然这拍无效，但仍要推进 sequence / timestamp 观察窗，
        // * 否则下一拍会因为持续拿旧参考做比较而重复上报时序异常。
        processor->previous_sequence = sequence;
        processor->previous_timestamp_ms = timestamp_ms;
        processor->previous_sample_seen = 1U;
        return;
    }

    // 阶段 3：样本通过门控后，才允许真正更新 baseline。
    if(processor->valid_sample_count == 0U) {
        // * 第一笔可信样本直接建立基线，避免冷启动阶段出现向 0 缓慢爬升的假过渡过程。
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
        // * 预热阶段窗口尚未填满，分母取 smooth_count，避免人为补零拉低振幅。
        processor->smooth_buffer[processor->smooth_index] = ac_ppg;
        processor->smooth_sum += ac_ppg;
        processor->smooth_count++;
    } else {
        // * 窗口已满后按环形缓冲区替换最旧样本，保持 O(1) 更新成本。
        processor->smooth_sum -= processor->smooth_buffer[processor->smooth_index];
        processor->smooth_buffer[processor->smooth_index] = ac_ppg;
        processor->smooth_sum += ac_ppg;
    }

    processor->smooth_index++;
    if(processor->smooth_index >= PPG_SMOOTH_WINDOW_SIZE) {//环形缓冲区 索引 处理
        processor->smooth_index = 0U;
    }

    filtered_ppg = processor->smooth_sum / (int32_t)processor->smooth_count;

    // 阶段 5：更新统计量与状态位。
    processor->valid_sample_count++;
    if(processor->valid_sample_count <= PPG_WARMUP_SAMPLE_COUNT) {
        // * 预热期内即便样本有效，也不代表基线和滑窗已经达到最佳稳定状态。
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

        // * 对于完全无效的样本，也要记住它发生过的时刻，
        // * 这样后续 sequence / timestamp 检查才基于真实采样节奏而不是理想节奏。
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
    // * 这样既能容忍 RTOS 调度抖动，也能在未来采样周期调整时继续复用同一套判定逻辑。
    target_q8 = delta_ms << 8U;
    if(processor->expected_interval_ms_q8 == 0U) {
        processor->expected_interval_ms_q8 = target_q8;
    } else {
        processor->expected_interval_ms_q8 =
            (processor->expected_interval_ms_q8 * 7U + target_q8) / 8U;//一阶低通滤波 新值 = 旧值 × (1-α) + 测量值 × α
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
    // * 只有向前跳号时才能推算明确丢了多少拍；倒退和重复只能记异常次数，不能累计 lost_sample_count。
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

    delta_ms = timestamp_ms - processor->previous_timestamp_ms;//计算两点数据时间间隔
    dynamic_threshold_ms = PPG_MAX_TIMESTAMP_GAP_MS;//获得用来比较的 时间间隔 阈值 75
    if(processor->expected_interval_ms_q8 != 0U) {
        uint32_t expected_ms = processor->expected_interval_ms_q8 >> 8U;
        uint32_t scaled_ms = expected_ms * 3U;
        if(scaled_ms > dynamic_threshold_ms) {
            dynamic_threshold_ms = scaled_ms;
        }
    }

    // 采样间隔明显超出运行时统计值，视为发生采样缺口。
    // * 阈值取“固定上限”和“运行时估计值 3 倍”中的较大者，
    // * 目的是在正常调度抖动下不过敏，但对真正长时间断流仍能快速报错。
    if(delta_ms > dynamic_threshold_ms) {
        processor->timestamp_gap_count++;
        return 1U;
    }

    PpgSignalProcessor_UpdateExpectedInterval(processor, delta_ms);// 一阶低通滤波 更新 expected_interval_ms_q8
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
    // * 这里隐含的物理含义是：接触更紧、直流量更大时，脉搏交流摆幅通常也会同步放大。
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
    // * 若只使用固定阈值，接触很弱时会过宽，接触很强时又可能过严。
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
