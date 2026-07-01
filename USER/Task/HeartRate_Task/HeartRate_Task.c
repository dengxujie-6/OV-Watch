#include "HeartRate_Task.h"

#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "heart_rate_algo.h"
#include "hwaccess.h"
#include "ppg_signal_processor.h"
#include "stm32f4xx_hal.h"
#include "task.h"

#define HEART_RATE_TASK_STARTUP_DELAY_MS   500U
#define HEART_RATE_TASK_SAMPLE_PERIOD_MS   25U
#define HEART_RATE_TASK_IDLE_PERIOD_MS     50U

static PpgSignalProcessor_t g_ppg_signal_processor;  // * PPG 前处理器对象，保存滤波与时序状态。
static HeartRateAlgo_t g_heart_rate_algo;            // * 心率算法对象，保存滑窗和 BPM 状态。

static void HeartRate_Task_UpdateCaches(const PpgProcessedSample_t * processed_sample);

/**
 * @brief 更新页面层可见的 raw PPG 与 BPM 缓存。
 *
 * @param processed_sample 当前拍已经完成前处理的样本。
 *                         传入 NULL 表示“这一拍没有新的有效前处理结果”。
 */
static void HeartRate_Task_UpdateCaches(const PpgProcessedSample_t * processed_sample)
{
    HeartRateAlgoDebugInfo_t debug_info;

    if(processed_sample == NULL) {
        // * 无效拍不覆盖最近一次有效 raw，只撤销当前 BPM 的有效标志。
        HwAccess_Em7028_UpdateHeartRateCache(0U, 0U);
        return;
    }

    HeartRateAlgo_GetDebugInfo(&g_heart_rate_algo, &debug_info);
    HwAccess_Em7028_UpdateRawCache(processed_sample->raw_ppg, processed_sample->valid);
    HwAccess_Em7028_UpdateHeartRateCache(debug_info.display_bpm_u8, debug_info.hr_valid);
}

/**
 * @brief EM7028 原始 PPG 采样任务。
 *
 * 这个任务只保留最简单的主链路：
 * 1. 固定周期采样；
 * 2. 调用 PPG 前处理层；
 * 3. 调用心率算法层；
 * 4. 更新页面可见缓存。
 *
 * @param argument FreeRTOS 任务参数，当前未使用。
 */
void HeartRate_Task(void *argument)
{
    TickType_t last_wake_tick = 0U;        // * vTaskDelayUntil() 的节拍基准点。
    uint8_t stream_active = 0U;            // * 标记当前是否已经进入一轮真正的测量会话。
    uint32_t next_sequence = 0U;           // * 当前测量会话内的样本逻辑序号。
    uint32_t sequence;
    uint32_t timestamp_ms;
    uint16_t raw_ppg = 0U;
    uint16_t retry_raw_ppg = 0U;
    uint8_t flags;
    int ret;
    int retry_ret;
    PpgProcessedSample_t processed_sample;

    (void)argument;

    osDelay(HEART_RATE_TASK_STARTUP_DELAY_MS);

    // * 初始化各结构体，清零并建立运行时状态变量。
    PpgSignalProcessor_Init(&g_ppg_signal_processor);
    HeartRateAlgo_Init(&g_heart_rate_algo);

    // * 如果底层提供传感器初始化接口，任务启动后先做一次初始化。
    if(HwAccess.em7028.init != NULL) {
        (void)HwAccess.em7028.init();
    }

    for(;;) {
        if((HwAccess.em7028.is_running == NULL) ||
           (HwAccess.em7028.is_running() == 0U)) {
            if(stream_active != 0U) {
                // * 停止测量时统一回到空闲态，避免上一轮窗口残留影响下一轮。
                HeartRateAlgo_Reset(&g_heart_rate_algo, HR_STATE_IDLE);
                HwAccess_Em7028_UpdateRawCache(0U, 0U);
                HwAccess_Em7028_UpdateHeartRateCache(0U, 0U);
                stream_active = 0U;
            }

            osDelay(HEART_RATE_TASK_IDLE_PERIOD_MS);
            continue;
        }

        if(stream_active == 0U) {
            // * 新一轮测量开始前，必须把前处理器、算法窗口和页面缓存复位干净。
            PpgSignalProcessor_Init(&g_ppg_signal_processor);
            HeartRateAlgo_Reset(&g_heart_rate_algo, HR_STATE_WARMUP);
            HwAccess_Em7028_UpdateRawCache(0U, 0U);
            HwAccess_Em7028_UpdateHeartRateCache(0U, 0U);
            last_wake_tick = xTaskGetTickCount();
            next_sequence = 0U;
            stream_active = 1U;
        }

        vTaskDelayUntil(&last_wake_tick, pdMS_TO_TICKS(HEART_RATE_TASK_SAMPLE_PERIOD_MS));

        timestamp_ms = HAL_GetTick();
        sequence = next_sequence;
        next_sequence++;
        flags = PPG_SAMPLE_FLAG_NONE;

        if(HwAccess.em7028.read_raw == NULL) {
            // ! 没有底层读接口时，本拍只能按无效样本推进，保持算法时序连续。
            PpgSignalProcessor_ProcessInvalidSample(&g_ppg_signal_processor,
                                                    sequence,
                                                    timestamp_ms,
                                                    PPG_SAMPLE_FLAG_I2C_ERROR,
                                                    &processed_sample);
            HeartRateAlgo_ProcessSample(&g_heart_rate_algo, &processed_sample);
            HeartRate_Task_UpdateCaches(NULL);
            continue;
        }

        ret = HwAccess.em7028.read_raw(&raw_ppg);
        if(ret != 0) {
            // ! 总线读失败时不能推进前处理状态，只能把这一拍标记为无效。
            flags |= PPG_SAMPLE_FLAG_I2C_ERROR;
            PpgSignalProcessor_ProcessInvalidSample(&g_ppg_signal_processor,
                                                    sequence,
                                                    timestamp_ms,
                                                    flags,
                                                    &processed_sample);
            HeartRateAlgo_ProcessSample(&g_heart_rate_algo, &processed_sample);
            HeartRate_Task_UpdateCaches(NULL);
            continue;
        }

        //  *如果判断 原始数据 raw_ppg 不是毛刺， 进入处理
        if(PpgSignalProcessor_IsGrossOutlier(&g_ppg_signal_processor, raw_ppg) == 0U) {
            PpgSignalProcessor_ProcessValidSample(&g_ppg_signal_processor,
                                                  sequence,
                                                  timestamp_ms,
                                                  raw_ppg,
                                                  flags,
                                                  &processed_sample);//  *这边对数据的判断，结果可能是无效的数据
            HeartRateAlgo_ProcessSample(&g_heart_rate_algo, &processed_sample);
            HeartRate_Task_UpdateCaches(&processed_sample);
            continue;
        }

        // * 第一次读数像毛刺时只给一次立即重读机会，避免把毛刺写进后续窗口。
        g_ppg_signal_processor.gross_outlier_count++;
        flags |= PPG_SAMPLE_FLAG_RETRY_USED;
        retry_ret = HwAccess.em7028.read_raw(&retry_raw_ppg);

        if((retry_ret == 0) &&
           (PpgSignalProcessor_IsGrossOutlier(&g_ppg_signal_processor, retry_raw_ppg) == 0U)) {
            flags |= PPG_SAMPLE_FLAG_RETRY_SUCCESS;
            PpgSignalProcessor_ProcessValidSample(&g_ppg_signal_processor,
                                                  sequence,
                                                  timestamp_ms,
                                                  retry_raw_ppg,
                                                  flags,
                                                  &processed_sample);
            HeartRateAlgo_ProcessSample(&g_heart_rate_algo, &processed_sample);
            HeartRate_Task_UpdateCaches(&processed_sample);
            continue;
        }

        if(retry_ret != 0) {
            flags |= PPG_SAMPLE_FLAG_I2C_ERROR;
        }

        // * 重读后仍不通过，则当前拍最终判为无效样本，不能污染 baseline / filtered 状态。
        flags |= PPG_SAMPLE_FLAG_GROSS_OUTLIER;
        PpgSignalProcessor_ProcessInvalidSample(&g_ppg_signal_processor,
                                                sequence,
                                                timestamp_ms,
                                                flags,
                                                &processed_sample);
        HeartRateAlgo_ProcessSample(&g_heart_rate_algo, &processed_sample);
        HeartRate_Task_UpdateCaches(NULL);
    }
}
