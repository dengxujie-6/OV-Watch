#include "HeartRate_Task.h"

#include <stdio.h>

#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "heart_rate_algo.h"
#include "hwaccess.h"
#include "ppg_signal_processor.h"
#include "ppg_uart_stream.h"
#include "stm32f4xx_hal.h"
#include "task.h"

#define HEART_RATE_DEBUG_ENABLE              0U  // 正式代码默认关闭诊断输出。
#define HEART_RATE_DIAG_UART_STREAM_BIT  (1UL << 0)
#define HEART_RATE_DIAG_INIT_REG_BIT     (1UL << 1)
#define HEART_RATE_DIAG_I2C_ERROR_BIT    (1UL << 2)
#define HEART_RATE_DIAG_STATS_REPORT_BIT (1UL << 3)
#define HEART_RATE_DIAG_BPM_SUMMARY_BIT  (1UL << 4)
#define HEART_RATE_DIAG_ALL_BITS         (HEART_RATE_DIAG_UART_STREAM_BIT | \
                                          HEART_RATE_DIAG_INIT_REG_BIT | \
                                          HEART_RATE_DIAG_I2C_ERROR_BIT | \
                                          HEART_RATE_DIAG_STATS_REPORT_BIT | \
                                          HEART_RATE_DIAG_BPM_SUMMARY_BIT)

#if (HEART_RATE_DEBUG_ENABLE != 0U)
#define HEART_RATE_DIAGNOSTIC_MASK HEART_RATE_DIAG_ALL_BITS
#else
#define HEART_RATE_DIAGNOSTIC_MASK 0UL
#endif

#define HEART_RATE_DIAG_HAS(mask_bit) (((HEART_RATE_DIAGNOSTIC_MASK) & (mask_bit)) != 0UL)

#define HEART_RATE_BPM_DEBUG_PERIOD_MS        1000U
#define HEART_RATE_BPM_DEBUG_TEXT_BUFFER_SIZE 192U

#if !HEART_RATE_DIAG_HAS(HEART_RATE_DIAG_UART_STREAM_BIT)
#define PpgUartStream_Init(stream)                 ((void)(stream))
#define PpgUartStream_Reset(stream)                ((void)(stream))
#define PpgUartStream_BeginSample(stream, tick_ms) (0UL)
#define PpgUartStream_RecordProcessedSample(stream, sample) ((void)(stream), (void)(sample))
#define PpgUartStream_RecordGrossOutlier(stream)   ((void)(stream))
#define PpgUartStream_RecordRetrySuccess(stream)   ((void)(stream))
#define PpgUartStream_RecordRetryFail(stream)      ((void)(stream))
#define PpgUartStream_RecordHrs1Ctrl(stream, expected, actual) \
    ((void)(stream), (void)(expected), (void)(actual))
#define PpgUartStream_RecordI2cError(stream, status, error) \
    ((void)(stream), (void)(status), (void)(error))
#define PpgUartStream_GetStats(stream, stats)      ((void)(stream), (void)(stats))
#define PpgUartStream_PushMetaText(stream, text, length) (0)
#define PpgUartStream_PushSample(stream, sample)   (0)
#define PpgUartStream_RequestFlush(stream)         ((void)(stream))
#define PpgUartStream_IsIdle(stream)               (1U)
#define PpgUartStream_BindTxTask(stream, handle)   ((void)(stream), (void)(handle))
#define PpgUartStream_Process(stream)              ((void)(stream))
#endif

#if !HEART_RATE_DIAG_HAS(HEART_RATE_DIAG_BPM_SUMMARY_BIT)
#define HeartRate_Task_MaybeSendBpmDebug(timestamp_ms, next_debug_tick_ms) \
    ((void)(timestamp_ms), (void)(next_debug_tick_ms))
#endif

#define HEART_RATE_EM7028_HRS1_CTRL_REG        0x0DU
#define HEART_RATE_EM7028_HRS1_CTRL_EXPECTED   0x47U
#define HEART_RATE_TASK_STARTUP_DELAY_MS       500U
#define HEART_RATE_TASK_SAMPLE_PERIOD_MS       25U
#define HEART_RATE_TASK_IDLE_PERIOD_MS         50U
#define HEART_RATE_TX_TASK_WAIT_MS             10U
#define HEART_RATE_TX_TASK_IDLE_MS             1000U
#define HEART_RATE_STREAM_DRAIN_WAIT_MS        10U
#define HEART_RATE_STREAM_DRAIN_TIMEOUT_MS     2000U
#define HEART_RATE_STATS_TEXT_BUFFER_SIZE      256U

static PpgUartStream_t g_ppg_uart_stream;           // 诊断串流对象。
static PpgSignalProcessor_t g_ppg_signal_processor; // PPG 前处理器对象。
static HeartRateAlgo_t g_heart_rate_algo;           // 心率算法对象。

static void HeartRate_Task_RegisterBluetoothHooks(void);
static void HeartRate_Task_RecordInitDiagnostics(void);
static void HeartRate_Task_RecordReadError(void);
#if HEART_RATE_DIAG_HAS(HEART_RATE_DIAG_BPM_SUMMARY_BIT)
static void HeartRate_Task_MaybeSendBpmDebug(uint32_t timestamp_ms,
                                             uint32_t * next_debug_tick_ms);
#endif
static void HeartRate_Task_SendStatsReport(void);
static void HeartRate_Task_UpdateCaches(const PpgProcessedSample_t * processed_sample);

/**
 * @brief 注册蓝牙串口 DMA 完成与错误回调。
 *
 * 阶段位置：
 * 这是“诊断链路初始化”的入口，不参与心率算法主逻辑。
 *
 * 数据流：
 * HeartRate_Task / PpgUartStream -> UART DMA -> ISR 回调 -> PpgUartStream
 *
 * 正式代码关闭串口诊断时，这个函数不做任何事。
 */
static void HeartRate_Task_RegisterBluetoothHooks(void)
{
#if HEART_RATE_DIAG_HAS(HEART_RATE_DIAG_UART_STREAM_BIT)
    // 如果底层暴露了发送完成钩子，就把串流 ISR 回调注册进去。
    if(HwAccess.bluetooth.register_tx_complete_hook != NULL) {
        HwAccess.bluetooth.register_tx_complete_hook(PpgUartStream_OnTxCompleteFromIsr,
                                                     &g_ppg_uart_stream);
    }

    // 如果底层暴露了 UART 错误钩子，也注册到同一个串流对象。
    if(HwAccess.bluetooth.register_error_hook != NULL) {
        HwAccess.bluetooth.register_error_hook(PpgUartStream_OnUartErrorFromIsr,
                                               &g_ppg_uart_stream);
    }
#endif
}

/**
 * @brief 更新页面层可见的 raw PPG 与 BPM 缓存。
 *
 * 页面层不直接访问任务对象和算法对象，而是统一通过 HwAccess 缓存取值。
 * 这样做的好处是 UI 层只关心“最近一次对外可见结果”，
 * 不需要知道当前任务到底处于哪一拍、也不需要碰算法内部状态。
 *
 * 数据流：
 * processed_sample->raw_ppg     -> raw cache -> 页面原始 PPG 显示
 * g_heart_rate_algo.display_bpm -> bpm cache -> 页面心率显示
 * g_heart_rate_algo.hr_valid    -> bpm valid -> 页面判断当前心率是否可信
 *
 * @param processed_sample 当前拍已经完成前处理的样本。
 *                         传入 NULL 表示“这一拍没有新的有效前处理结果”。
 */
static void HeartRate_Task_UpdateCaches(const PpgProcessedSample_t * processed_sample)
{
    HeartRateAlgoDebugInfo_t debug_info;  // 用于接收算法层对外暴露的调试快照。

    if(processed_sample == NULL) {
        // 故意不清空最近一次有效 raw，避免页面因为单拍无效而立刻闪回 Waiting raw。
        HwAccess_Em7028_UpdateHeartRateCache(0U, 0U);
        return;
    }

    // 先从算法层取出当前显示 BPM 和有效标志。
    HeartRateAlgo_GetDebugInfo(&g_heart_rate_algo, &debug_info);
    // raw 缓存直接反映这一次前处理后的样本内容。
    HwAccess_Em7028_UpdateRawCache(processed_sample->raw_ppg, processed_sample->valid);
    // 心率缓存使用算法层当前输出的显示值，而不是直接使用样本本身。
    HwAccess_Em7028_UpdateHeartRateCache(debug_info.display_bpm_u8, debug_info.hr_valid);
}

/**
 * @brief 每秒输出一次窗口级心率调试摘要。
 *
 * 这不是逐样本 CSV，而是对“当前心率窗口状态”的概括：
 * - 窗口有效率够不够
 * - 自相关峰值是否稳定
 * - candidate_bpm 和 display_bpm 当前分别是多少
 *
 * 正式代码默认关闭。
 *
 * @param timestamp_ms 当前采样拍点对应的系统时间戳。
 * @param next_debug_tick_ms 下一次允许输出摘要的时间点。
 */
#if HEART_RATE_DIAG_HAS(HEART_RATE_DIAG_BPM_SUMMARY_BIT)
static void HeartRate_Task_MaybeSendBpmDebug(uint32_t timestamp_ms, uint32_t * next_debug_tick_ms)
{
    char report_buffer[HEART_RATE_BPM_DEBUG_TEXT_BUFFER_SIZE];  // 调试文本拼接缓冲区。
    HeartRateAlgoDebugInfo_t debug_info;                        // 算法层窗口状态快照。
    int text_length;                                            // snprintf 返回的文本长度。

    // 调用方若没有提供节拍状态指针，就无法安全输出周期摘要。
    if(next_debug_tick_ms == NULL) {
        return;
    }

    if(*next_debug_tick_ms == 0U) {
        // 首次仅建立节拍，不立即输出，给算法预热并避免日志过密。
        *next_debug_tick_ms = timestamp_ms + HEART_RATE_BPM_DEBUG_PERIOD_MS;
        return;
    }

    // 还没到下一次允许输出的时间点时，直接返回。
    if((int32_t)(timestamp_ms - *next_debug_tick_ms) < 0) {
        return;
    }

    // 读取算法层当前窗口状态，准备序列化为单行调试文本。
    HeartRateAlgo_GetDebugInfo(&g_heart_rate_algo, &debug_info);
    text_length = snprintf(report_buffer,
                           sizeof(report_buffer),
                           "PPG_BPM_DEBUG,"
                           "sequence_gap_count=%lu,"
                           "timestamp_gap_count=%lu,"
                           "raw_reject_count=%lu,"
                           "flag_reject_count=%lu,"
                           "retry_success_count=%lu,"
                           "window_valid_ratio=%.3f,"
                           "autocorr_peak=%.3f,"
                           "candidate_bpm=%.2f,"
                           "display_bpm=%.2f,"
                           "hr_valid=%u\r\n",
                           (unsigned long)g_ppg_signal_processor.sequence_gap_count,
                           (unsigned long)g_ppg_signal_processor.timestamp_gap_count,
                           (unsigned long)g_ppg_signal_processor.raw_reject_count,
                           (unsigned long)g_ppg_signal_processor.flag_reject_count,
                           (unsigned long)g_ppg_signal_processor.retry_success_count,
                           (double)debug_info.window_valid_ratio,
                           (double)debug_info.autocorr_peak,
                           (double)debug_info.candidate_bpm,
                           (double)debug_info.display_bpm,
                           (unsigned int)debug_info.hr_valid);
    // 只有文本成功写入且没有超过缓冲区时，才推送到串流模块。
    if((text_length > 0) && ((uint32_t)text_length < sizeof(report_buffer))) {
        (void)PpgUartStream_PushMetaText(&g_ppg_uart_stream,
                                         report_buffer,
                                         (uint16_t)text_length);
    }

    // 无论本次是否成功推送，都推进下一次摘要输出时间。
    *next_debug_tick_ms = timestamp_ms + HEART_RATE_BPM_DEBUG_PERIOD_MS;
}
#endif

/**
 * @brief 记录 EM7028 HRS1 控制寄存器读回值。
 *
 * 这类信息属于“初始化诊断”，不是正式算法主流程的一部分。
 * 正式代码默认关闭。
 */
static void HeartRate_Task_RecordInitDiagnostics(void)
{
#if HEART_RATE_DIAG_HAS(HEART_RATE_DIAG_INIT_REG_BIT)
    uint8_t readback = 0U;  // 保存 HRS1_CTRL 寄存器读回值。
    int ret;                // 保存底层寄存器读取接口返回值。

    // 如果底层没有暴露 read_reg，就只能记录“期望值存在、实际值未知”。
    if(HwAccess.em7028.read_reg == NULL) {
        PpgUartStream_RecordHrs1Ctrl(&g_ppg_uart_stream,
                                     HEART_RATE_EM7028_HRS1_CTRL_EXPECTED,
                                     0U);
        return;
    }

    // 从底层读取 HRS1 控制寄存器当前值。
    ret = HwAccess.em7028.read_reg(HEART_RATE_EM7028_HRS1_CTRL_REG, &readback);
    // 读取失败时，把读回值清零，避免上传未定义脏值。
    if(ret != 0) {
        readback = 0U;
    }

    // 把期望值和实际值一起送入诊断串流，方便后续对比初始化状态。
    PpgUartStream_RecordHrs1Ctrl(&g_ppg_uart_stream,
                                 HEART_RATE_EM7028_HRS1_CTRL_EXPECTED,
                                 readback);
#endif
}

/**
 * @brief 读取最近一次 I2C 访问错误状态，并写入串流诊断。
 *
 * 这里不改变当前样本本身，只负责把“底层记录的最近一次错误原因”
 * 带到调试串流里，方便回看失败时是 HAL 状态还是 I2C 错误码异常。
 *
 * 正式代码默认关闭。
 */
static void HeartRate_Task_RecordReadError(void)
{
#if HEART_RATE_DIAG_HAS(HEART_RATE_DIAG_I2C_ERROR_BIT)
    HAL_StatusTypeDef hal_status = HAL_ERROR;  // 默认先按通用 HAL_ERROR 处理。
    uint32_t hal_error = 0U;                   // 默认错误码先清零。

    // 如果底层提供最近一次 I2C 状态读取接口，就把它取出来。
    if(HwAccess.em7028.get_last_i2c_status != NULL) {
        hal_status = (HAL_StatusTypeDef)HwAccess.em7028.get_last_i2c_status();
    }

    // 如果底层提供最近一次 I2C 错误码读取接口，也一并取出来。
    if(HwAccess.em7028.get_last_i2c_error != NULL) {
        hal_error = HwAccess.em7028.get_last_i2c_error();
    }

    // 最终统一写入诊断串流，而不是在任务里直接打印。
    PpgUartStream_RecordI2cError(&g_ppg_uart_stream, hal_status, hal_error);
#endif
}

/**
 * @brief 停止采样后，通过 UART 串流输出一次统计摘要。
 *
 * 设计意图：
 * 1. 先把前面的逐样本 CSV 尽量发完。
 * 2. 再补一条测量周期总结。
 * 3. 再等待一次，确保总结本身也尽量发出去。
 *
 * 正式代码默认关闭。
 */
static void HeartRate_Task_SendStatsReport(void)
{
#if HEART_RATE_DIAG_HAS(HEART_RATE_DIAG_STATS_REPORT_BIT)
    char report_buffer[HEART_RATE_STATS_TEXT_BUFFER_SIZE];  // 统计摘要文本缓冲区。
    PpgUartStreamStats_t stats;                             // 串流模块统计快照。
    TickType_t stop_wait_tick;                              // 等待排空开始时刻。
    int text_length;                                        // 文本格式化长度。

    // 先从串流模块取出当前统计快照。
    PpgUartStream_GetStats(&g_ppg_uart_stream, &stats);

    // 把当前统计组织成一条上位机容易解析的 CSV 文本。
    text_length = snprintf(report_buffer,
                           sizeof(report_buffer),
                           "PPG_PROCESS_STATS,"
                           "ok=%lu,"
                           "i2c_err=%lu,"
                           "gross_outlier=%lu,"
                           "retry_success=%lu,"
                           "retry_fail=%lu,"
                           "valid_processed=%lu,"
                           "invalid_processed=%lu,"
                           "latest_baseline=%ld,"
                           "latest_ac=%ld,"
                           "latest_filtered=%ld,"
                           "pushed=%lu,"
                           "sent=%lu,"
                           "dropped=%lu,"
                           "dma_error=%lu\r\n",
                           (unsigned long)stats.sample_ok_count,
                           (unsigned long)stats.sample_i2c_error_count,
                           (unsigned long)stats.gross_outlier_count,
                           (unsigned long)stats.retry_success_count,
                           (unsigned long)stats.retry_fail_count,
                           (unsigned long)stats.valid_processed_count,
                           (unsigned long)stats.invalid_processed_count,
                           (long)stats.latest_baseline_ppg,
                           (long)stats.latest_ac_ppg,
                           (long)stats.latest_filtered_ppg,
                           (unsigned long)stats.pushed_samples,
                           (unsigned long)stats.sent_samples,
                           (unsigned long)stats.dropped_samples,
                           (unsigned long)stats.dma_error_count);
    // 文本格式化失败或超长时，直接放弃本次统计输出。
    if((text_length <= 0) || ((uint32_t)text_length >= sizeof(report_buffer))) {
        return;
    }

    // 统计摘要也必须走统一串流通道，不能绕过当前双缓冲发送链路。
    if(PpgUartStream_PushMetaText(&g_ppg_uart_stream,
                                  report_buffer,
                                  (uint16_t)text_length) != 0) {
        return;
    }

    // 主动请求 flush，把当前未满的数据块也尽快发出去。
    PpgUartStream_RequestFlush(&g_ppg_uart_stream);
    // 记录等待开始时刻，用于后续超时保护。
    stop_wait_tick = xTaskGetTickCount();

    // 只要串流还未空闲，就继续周期性等待。
    while(PpgUartStream_IsIdle(&g_ppg_uart_stream) == 0U) {
        osDelay(HEART_RATE_STREAM_DRAIN_WAIT_MS);
        // 到达超时上限后退出，避免在停止测量阶段无限阻塞。
        if((xTaskGetTickCount() - stop_wait_tick) >=
           pdMS_TO_TICKS(HEART_RATE_STREAM_DRAIN_TIMEOUT_MS)) {
            break;
        }
    }
#endif
}

/**
 * @brief EM7028 原始 PPG 采样任务。
 *
 * 这个任务是整条心率链路的总入口，正式职责只保留四件事：
 * 1. 固定周期采样。
 * 2. 调用 PPG 前处理层。
 * 3. 调用心率算法层。
 * 4. 维护页面可见结果缓存与测量会话起止。
 *
 * 推荐按下面阶段理解数据流：
 * 阶段 1：任务启动与软件对象初始化。
 * 阶段 2：等待传感器进入 running 状态。
 * 阶段 3：固定周期读取 raw_ppg。
 * 阶段 4：决定当前 raw 是否直接前处理、重读一次、还是直接判无效。
 * 阶段 5：把 processed_sample 送入心率算法。
 * 阶段 6：把结果同步到页面缓存，以及可选的调试链路。
 *
 * 注意：
 * 心率算法明确运行在任务上下文，不在中断中执行。
 */
void HeartRate_Task(void *argument)
{
    TickType_t last_wake_tick = 0U;        // vTaskDelayUntil() 的节拍基准点。
    TickType_t stop_wait_tick = 0U;        // 停止测量后等待串流排空的起始 tick。
    uint8_t stream_active = 0U;            // 标记当前是否已经进入一轮真正的测量会话。
    uint32_t next_debug_tick_ms = 0U;      // 控制 BPM 摘要下一次允许输出的时间点。
    uint32_t sequence;                     // 当前样本逻辑序号。
    uint32_t timestamp_ms;                 // 当前拍点时间戳。
    uint16_t raw_ppg = 0U;                 // 第一次读取到的原始 PPG。
    uint16_t retry_raw_ppg = 0U;           // 重读得到的原始 PPG。
    uint8_t flags;                         // 当前样本附加标志位。
    int ret;                               // 第一次 read_raw 返回值。
    int retry_ret;                         // 重读 read_raw 返回值。
    PpgProcessedSample_t processed_sample; // 当前拍最终写给算法层的统一样本对象。

    // 当前任务不使用入口参数。
    (void)argument;

    // 阶段 1：上电后先等待外设稳定，再初始化软件状态。
    osDelay(HEART_RATE_TASK_STARTUP_DELAY_MS);

    // 初始化软件对象；即使正式代码关闭调试，也保留统一调用结构。
    // *初始化各结构体， 清零 & 初始化结构体内部状态变量
    PpgUartStream_Init(&g_ppg_uart_stream);
    PpgSignalProcessor_Init(&g_ppg_signal_processor);
    HeartRateAlgo_Init(&g_heart_rate_algo);
    HeartRate_Task_RegisterBluetoothHooks();

    // 如果底层提供传感器初始化接口，任务启动后先做一次初始化。
    if(HwAccess.em7028.init != NULL) {
        (void)HwAccess.em7028.init();
        HeartRate_Task_RecordInitDiagnostics();
    }

    // 任务主循环持续运行，负责驱动整条心率采样链路。
    for(;;) {
        // 先判断传感器当前是否处于 running 状态。
        if((HwAccess.em7028.is_running == NULL) ||
           (HwAccess.em7028.is_running() == 0U)) {
            // 如果上一轮已经处于测量态，而现在停止了，就要做一次收尾。
            if(stream_active != 0U) {
                // 传感器从 running 退出时，先尽量冲刷掉调试串流残留内容。
                PpgUartStream_RequestFlush(&g_ppg_uart_stream);
                // 记录等待排空的起始 tick。
                stop_wait_tick = xTaskGetTickCount();

                // 持续等待串流模块回到空闲态。
                while(PpgUartStream_IsIdle(&g_ppg_uart_stream) == 0U) {
                    osDelay(HEART_RATE_STREAM_DRAIN_WAIT_MS);
                    // 如果等待时间过长，就强制结束等待。
                    if((xTaskGetTickCount() - stop_wait_tick) >=
                       pdMS_TO_TICKS(HEART_RATE_STREAM_DRAIN_TIMEOUT_MS)) {
                        break;
                    }
                }

                // 测量会话结束后，算法和页面缓存都要回到“停止测量”状态。
                HeartRate_Task_SendStatsReport();
                HeartRateAlgo_Reset(&g_heart_rate_algo, HR_STATE_IDLE);
                HwAccess_Em7028_UpdateRawCache(0U, 0U);
                HwAccess_Em7028_UpdateHeartRateCache(0U, 0U);
                stream_active = 0U;
                next_debug_tick_ms = 0U;
            }

            // 未运行时不忙轮询，只低频等待。
            osDelay(HEART_RATE_TASK_IDLE_PERIOD_MS);
            continue;
        }

        // 第一次进入 running 状态时，要先把所有状态机重置干净。
        if(stream_active == 0U) {
            // 新一轮测量开始时，先整体复位串流、前处理器、算法和页面缓存。
            PpgUartStream_Reset(&g_ppg_uart_stream);
            PpgSignalProcessor_Init(&g_ppg_signal_processor);
            HeartRateAlgo_Reset(&g_heart_rate_algo, HR_STATE_WARMUP);
            HwAccess_Em7028_UpdateRawCache(0U, 0U);
            HwAccess_Em7028_UpdateHeartRateCache(0U, 0U);
            last_wake_tick = xTaskGetTickCount();
            stream_active = 1U;
            next_debug_tick_ms = 0U;
            HeartRate_Task_RecordInitDiagnostics();
        }

        // 阶段 3：按固定周期采样，vTaskDelayUntil() 更适合稳定周期任务。
        vTaskDelayUntil(&last_wake_tick, pdMS_TO_TICKS(HEART_RATE_TASK_SAMPLE_PERIOD_MS));

        // 读取当前系统 tick，作为本拍时间戳。
        timestamp_ms = HAL_GetTick();
        // 无论成功失败，本拍都先占一个 sequence，保证下游能感知真实时序。
        sequence = PpgUartStream_BeginSample(&g_ppg_uart_stream, timestamp_ms);
        // 每一拍都从空标志开始，后面再按实际情况补充。
        flags = PPG_SAMPLE_FLAG_NONE;

        // 底层若没有 read_raw 接口，则这一拍无法得到原始样本。
        if(HwAccess.em7028.read_raw == NULL) {
            HeartRate_Task_RecordReadError();
            PpgSignalProcessor_ProcessInvalidSample(&g_ppg_signal_processor,
                                                    sequence,
                                                    timestamp_ms,
                                                    PPG_SAMPLE_FLAG_I2C_ERROR,
                                                    &processed_sample);
            // 底层没有 read_raw 接口时，本拍只能按无效样本推进，保持算法时序连续。
            HeartRateAlgo_ProcessSample(&g_heart_rate_algo, &processed_sample);
            HeartRate_Task_UpdateCaches(NULL);
            PpgUartStream_RecordProcessedSample(&g_ppg_uart_stream, &processed_sample);
            (void)PpgUartStream_PushSample(&g_ppg_uart_stream, &processed_sample);
            HeartRate_Task_MaybeSendBpmDebug(timestamp_ms, &next_debug_tick_ms);
            continue;
        }

        // 先尝试读取本拍的原始 PPG。
        ret = HwAccess.em7028.read_raw(&raw_ppg);
        if(ret != 0) {
            HeartRate_Task_RecordReadError();
            // 第一次读失败时，该拍不能进入前处理状态更新。
            flags |= PPG_SAMPLE_FLAG_I2C_ERROR;
            PpgSignalProcessor_ProcessInvalidSample(&g_ppg_signal_processor,
                                                    sequence,
                                                    timestamp_ms,
                                                    flags,
                                                    &processed_sample);
            HeartRateAlgo_ProcessSample(&g_heart_rate_algo, &processed_sample);
            HeartRate_Task_UpdateCaches(NULL);
            PpgUartStream_RecordProcessedSample(&g_ppg_uart_stream, &processed_sample);
            (void)PpgUartStream_PushSample(&g_ppg_uart_stream, &processed_sample);
            HeartRate_Task_MaybeSendBpmDebug(timestamp_ms, &next_debug_tick_ms);
            continue;
        }

        // 如果当前原始值不是明显毛刺，就直接走正常样本路径。
        if(PpgSignalProcessor_IsGrossOutlier(&g_ppg_signal_processor, raw_ppg) == 0U) {
            PpgSignalProcessor_ProcessValidSample(&g_ppg_signal_processor,
                                                  sequence,
                                                  timestamp_ms,
                                                  raw_ppg,
                                                  flags,
                                                  &processed_sample);
            // 正常样本路径：前处理完成后，继续推进算法并同步页面缓存。
            HeartRateAlgo_ProcessSample(&g_heart_rate_algo, &processed_sample);
            HeartRate_Task_UpdateCaches(&processed_sample);
            PpgUartStream_RecordProcessedSample(&g_ppg_uart_stream, &processed_sample);
            (void)PpgUartStream_PushSample(&g_ppg_uart_stream, &processed_sample);
            HeartRate_Task_MaybeSendBpmDebug(timestamp_ms, &next_debug_tick_ms);
            continue;
        }

        // 第一次读数像毛刺时，只给一次立即重读机会，避免把毛刺写进后续窗口。
        PpgUartStream_RecordGrossOutlier(&g_ppg_uart_stream);
        // 同步累计 gross outlier 计数。
        g_ppg_signal_processor.gross_outlier_count++;
        // 记录本拍已经用过重读机会。
        flags |= PPG_SAMPLE_FLAG_RETRY_USED;
        // 立即重读一次原始值。
        retry_ret = HwAccess.em7028.read_raw(&retry_raw_ppg);

        // 如果重读成功且不再像毛刺，就把本拍恢复为有效样本。
        if((retry_ret == 0) &&
           (PpgSignalProcessor_IsGrossOutlier(&g_ppg_signal_processor, retry_raw_ppg) == 0U)) {
            PpgUartStream_RecordRetrySuccess(&g_ppg_uart_stream);
            // 重读成功且不再像毛刺时，当前拍仍按有效样本处理，但保留“使用过重读”的痕迹。
            flags |= PPG_SAMPLE_FLAG_RETRY_SUCCESS;
            PpgSignalProcessor_ProcessValidSample(&g_ppg_signal_processor,
                                                  sequence,
                                                  timestamp_ms,
                                                  retry_raw_ppg,
                                                  flags,
                                                  &processed_sample);
            HeartRateAlgo_ProcessSample(&g_heart_rate_algo, &processed_sample);
            HeartRate_Task_UpdateCaches(&processed_sample);
            PpgUartStream_RecordProcessedSample(&g_ppg_uart_stream, &processed_sample);
            (void)PpgUartStream_PushSample(&g_ppg_uart_stream, &processed_sample);
            HeartRate_Task_MaybeSendBpmDebug(timestamp_ms, &next_debug_tick_ms);
            continue;
        }

        // 如果重读本身失败，还要补记一次 I2C 访问失败。
        if(retry_ret != 0) {
            HeartRate_Task_RecordReadError();
            // 重读本身失败时，在已有 flags 上补一个 I2C_ERROR。
            flags |= PPG_SAMPLE_FLAG_I2C_ERROR;
        }

        // 重读后仍不通过，则当前拍最终判为无效样本，不能污染 baseline / filtered 状态。
        PpgUartStream_RecordRetryFail(&g_ppg_uart_stream);
        flags |= PPG_SAMPLE_FLAG_GROSS_OUTLIER;
        PpgSignalProcessor_ProcessInvalidSample(&g_ppg_signal_processor,
                                                sequence,
                                                timestamp_ms,
                                                flags,
                                                &processed_sample);
        // 即使最终无效，也继续推进算法层，保持窗口时序一致。
        HeartRateAlgo_ProcessSample(&g_heart_rate_algo, &processed_sample);
        // 页面缓存本拍只更新“当前无新可信结果”的状态。
        HeartRate_Task_UpdateCaches(NULL);
        // 诊断链路仍然记录这一个无效样本。
        PpgUartStream_RecordProcessedSample(&g_ppg_uart_stream, &processed_sample);
        // 尝试把这一拍样本压入串流输出。
        (void)PpgUartStream_PushSample(&g_ppg_uart_stream, &processed_sample);
        // 根据节拍条件决定是否补发 BPM 摘要。
        HeartRate_Task_MaybeSendBpmDebug(timestamp_ms, &next_debug_tick_ms);
    }
}

/**
 * @brief 心率串口 DMA 发送任务。
 *
 * 这个任务只服务于调试串流，不参与心率算法主流程。
 *
 * 正式代码关闭串口诊断时：
 * - 不再 10ms 周期性推进串流状态机；
 * - 只低频休眠，避免无意义 CPU 占用。
 */
void HeartRate_UartTx_Task(void *argument)
{
    // 当前发送任务不使用入口参数。
    (void)argument;

#if HEART_RATE_DIAG_HAS(HEART_RATE_DIAG_UART_STREAM_BIT)
    // 把当前任务句柄告诉串流模块，方便 DMA 完成或出错时由 ISR 唤醒。
    PpgUartStream_BindTxTask(&g_ppg_uart_stream, xTaskGetCurrentTaskHandle());

    // 持续运行，专门负责推进 UART DMA 串流状态机。
    for(;;) {
        // 等待 ISR 通知，或者在超时后主动醒来推进状态机。
        (void)ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(HEART_RATE_TX_TASK_WAIT_MS));
        // 有通知或超时就推进一次 UART DMA 串流状态机。
        PpgUartStream_Process(&g_ppg_uart_stream);
    }
#else
    // 关闭诊断串流时，这个任务只低频休眠，避免空转占用 CPU。
    for(;;) {
        osDelay(HEART_RATE_TX_TASK_IDLE_MS);
    }
#endif
}
