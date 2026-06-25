#include "HeartRate_Task.h"

#include <stdio.h>

#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "hwaccess.h"
#include "ppg_signal_processor.h"
#include "ppg_uart_stream.h"
#include "stm32f4xx_hal.h"
#include "task.h"

#define HEART_RATE_UART_STREAM_ENABLE 0U

#if (HEART_RATE_UART_STREAM_ENABLE == 0U)
#define PpgUartStream_Init(stream)                 ((void)(stream))
#define PpgUartStream_Reset(stream)                ((void)(stream))
#define PpgUartStream_BeginSample(stream, tick_ms) (0UL)
#define PpgUartStream_RecordProcessedSample(stream, sample) ((void)(stream), (void)(sample))
#define PpgUartStream_RecordGrossOutlier(stream)   ((void)(stream))
#define PpgUartStream_RecordRetrySuccess(stream)   ((void)(stream))
#define PpgUartStream_RecordRetryFail(stream)      ((void)(stream))
#define PpgUartStream_PushSample(stream, sample)   (0)
#define PpgUartStream_RequestFlush(stream)         ((void)(stream))
#define PpgUartStream_IsIdle(stream)               (1U)
#define PpgUartStream_BindTxTask(stream, handle)   ((void)(stream), (void)(handle))
#define PpgUartStream_Process(stream)              ((void)(stream))
#endif

// HRS1 控制寄存器地址，用来在启动前后做一次只读诊断。
#define HEART_RATE_EM7028_HRS1_CTRL_REG         0x0DU
// 当前项目期望的 HRS1_CTRL 寄存器值，便于确认 BSP 初始化是否符合预期。
#define HEART_RATE_EM7028_HRS1_CTRL_EXPECTED    0x47U

// 心率任务启动后的初始延时，避免系统刚起机时外设仍在初始化。
#define HEART_RATE_TASK_STARTUP_DELAY_MS        500U
// PPG 固定采样周期，当前要求保持 25ms，即 40Hz。
#define HEART_RATE_TASK_SAMPLE_PERIOD_MS        25U
// 心率监测未运行时的空闲轮询周期。
#define HEART_RATE_TASK_IDLE_PERIOD_MS          50U
// 串流发送任务等待通知的超时时间。
#define HEART_RATE_TX_TASK_WAIT_MS              10U
// 停止采样后，等待串流排空时每次检查之间的延时。
#define HEART_RATE_STREAM_DRAIN_WAIT_MS         10U
// 停止采样后，等待串流排空的最长超时时间。
#define HEART_RATE_STREAM_DRAIN_TIMEOUT_MS      2000U
// 统计文本格式化缓冲区大小。
#define HEART_RATE_STATS_TEXT_BUFFER_SIZE       256U

// UART DMA 双缓冲串流对象，负责承接 CSV 行并异步发送。
static PpgUartStream_t g_ppg_uart_stream;
// PPG 信号处理器对象，负责异常样本判断后的基线与平滑处理。
static PpgSignalProcessor_t g_ppg_signal_processor;

static void HeartRate_Task_RegisterBluetoothHooks(void);
static void HeartRate_Task_RecordInitDiagnostics(void);
static void HeartRate_Task_RecordReadError(void);
static void HeartRate_Task_SendStatsReport(void);

/**
 * @brief 注册蓝牙 UART DMA 完成/错误回调。
 *
 * 这里不直接 include BSP 蓝牙头文件，而是通过 HwAccess 暴露的钩子接口，
 * 把串口 DMA 的完成/错误事件桥接到 PPG UART 双缓冲模块。
 * 这样任务层仍然只依赖 HwAccess，不越级碰 BSP。
 */
static void HeartRate_Task_RegisterBluetoothHooks(void)
{
    // 如果底层提供“DMA 发送完成”回调注册接口，就把它挂到串流模块。
    if(HwAccess.bluetooth.register_tx_complete_hook != NULL) {
        HwAccess.bluetooth.register_tx_complete_hook(PpgUartStream_OnTxCompleteFromIsr,
                                                     &g_ppg_uart_stream);
    }

    // 如果底层提供“UART 错误”回调注册接口，也同样挂到串流模块。
    if(HwAccess.bluetooth.register_error_hook != NULL) {
        HwAccess.bluetooth.register_error_hook(PpgUartStream_OnUartErrorFromIsr,
                                               &g_ppg_uart_stream);
    }
}

/**
 * @brief 记录当前 HRS1 控制寄存器读回值。
 *
 * 这个函数的目的不是控制 EM7028，而是做一次只读诊断：
 * - 期望值是 0x47；
 * - 实际值从寄存器读回；
 * - 最终写入串流统计，便于后续调试时核对 BSP 初始化是否正确。
 */
static void HeartRate_Task_RecordInitDiagnostics(void)
{
    // 用于保存寄存器读回值，默认先置 0。
    uint8_t readback = 0U;
    // 保存寄存器读取接口的返回值。
    int ret;

    // 如果当前没有寄存器读取接口，就只能把读回值记为 0。
    if(HwAccess.em7028.read_reg == NULL) {
        PpgUartStream_RecordHrs1Ctrl(&g_ppg_uart_stream,
                                     HEART_RATE_EM7028_HRS1_CTRL_EXPECTED,
                                     0U);
        return;
    }

    // 读取 HRS1_CTRL 当前值。
    ret = HwAccess.em7028.read_reg(HEART_RATE_EM7028_HRS1_CTRL_REG, &readback);
    // 如果读取失败，则把读回值清零，避免保留未定义脏数据。
    if(ret != 0) {
        readback = 0U;
    }

    // 把期望值与实际读回值记录进串流统计。
    PpgUartStream_RecordHrs1Ctrl(&g_ppg_uart_stream,
                                 HEART_RATE_EM7028_HRS1_CTRL_EXPECTED,
                                 readback);
}

/**
 * @brief 读取最近一次 I2C 访问错误并写入统计。
 *
 * 采样失败时，任务层不直接打印日志，而是把 HAL 状态与 HAL 错误码
 * 读出来并交给串流统计模块统一计数。
 */
static void HeartRate_Task_RecordReadError(void)
{
    // 默认先认为是通用 HAL_ERROR。
    HAL_StatusTypeDef hal_status = HAL_ERROR;
    // 保存最近一次软件 I2C/HAL 错误码。
    uint32_t hal_error = 0U;

    // 如果提供了最近一次 I2C HAL 状态读取接口，就把它取出来。
    if(HwAccess.em7028.get_last_i2c_status != NULL) {
        hal_status = (HAL_StatusTypeDef)HwAccess.em7028.get_last_i2c_status();
    }

    // 如果提供了最近一次 I2C HAL 错误码读取接口，也一并取出来。
    if(HwAccess.em7028.get_last_i2c_error != NULL) {
        hal_error = HwAccess.em7028.get_last_i2c_error();
    }

    // 最终把错误原因记入串流统计。
    PpgUartStream_RecordI2cError(&g_ppg_uart_stream, hal_status, hal_error);
}

/**
 * @brief 采样停止后，通过现有 UART DMA 双缓冲输出一次统计信息。
 *
 * 这里的关键点是：统计文本不能插进持续采样期间的 CSV 流里。
 * 因此顺序必须是：
 * 1. 前面的 CSV 先全部发完；
 * 2. 再单独压入一条统计文本；
 * 3. 再 flush 一次，确保统计本身也真正发出去。
 */
static void HeartRate_Task_SendStatsReport(void)
{
    // 用于拼接统计文本的本地字符缓冲区。
    char report_buffer[HEART_RATE_STATS_TEXT_BUFFER_SIZE];
    // 用于接收当前串流统计快照。
    PpgUartStreamStats_t stats;
    // 等待统计文本发送完成时的起始 tick。
    TickType_t stop_wait_tick;
    // snprintf 返回的文本长度。
    int text_length;

    // 从串流模块取出一份只读统计快照。
    PpgUartStream_GetStats(&g_ppg_uart_stream, &stats);

    // 把当前统计拼接成单行文本，便于上位机后处理。
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

    // 如果格式化失败，或者文本长度超过缓冲区，就直接放弃输出。
    if((text_length <= 0) || ((uint32_t)text_length >= sizeof(report_buffer))) {
        return;
    }

    // 统计文本也必须走同一套双缓冲链路，不能绕过串流模块直接发串口。
    if(PpgUartStream_PushText(&g_ppg_uart_stream,
                              report_buffer,
                              (uint16_t)text_length) != 0) {
        return;
    }

    // 请求把当前可能未满的缓冲块也立刻转成 READY 并发送出去。
    PpgUartStream_RequestFlush(&g_ppg_uart_stream);
    // 记录开始等待统计文本真正发完的时间。
    stop_wait_tick = xTaskGetTickCount();

    // 只要串流模块还不空闲，就周期性等待。
    while(PpgUartStream_IsIdle(&g_ppg_uart_stream) == 0U) {
        // 每次等待 10ms，避免忙轮询。
        osDelay(HEART_RATE_STREAM_DRAIN_WAIT_MS);
        // 如果等待超时，就退出，防止任务被无限卡住。
        if((xTaskGetTickCount() - stop_wait_tick) >=
           pdMS_TO_TICKS(HEART_RATE_STREAM_DRAIN_TIMEOUT_MS)) {
            break;
        }
    }
}

/**
 * @brief EM7028 原始 PPG 采样任务。
 *
 * 这个任务的职责是：
 * 1. 维持固定 25ms 周期；
 * 2. 读取原始 PPG；
 * 3. 处理 I2C 失败和异常跳变；
 * 4. 必要时立即重读一次；
 * 5. 调用信号处理器计算 baseline / ac / filtered；
 * 6. 把每个周期的结果都输出为一条 CSV。
 *
 * @param argument 任务参数，当前未使用。
 */
void HeartRate_Task(void *argument)
{
    // vTaskDelayUntil 依赖的“上一次唤醒时刻”。
    TickType_t last_wake_tick = 0U;
    // 停止采样后等待串流排空时使用的起始 tick。
    TickType_t stop_wait_tick = 0U;
    // 标记当前串流是否已经进入“活跃采样态”。
    uint8_t stream_active = 0U;
    // 当前采样周期对应的 sequence。
    uint32_t sequence;
    // 当前采样周期的毫秒级时间戳。
    uint32_t timestamp_ms;
    // 第一次读取到的原始 PPG。
    uint16_t raw_ppg;
    // 异常跳变时，单次重读得到的原始 PPG。
    uint16_t retry_raw_ppg;
    // 当前样本附加标志位。
    uint8_t flags;
    // 第一次 read_raw 的返回值。
    int ret;
    // 单次重读 read_raw 的返回值。
    int retry_ret;
    // 当前周期最终生成的处理后样本对象。
    PpgProcessedSample_t processed_sample;

    // 当前任务不使用 argument 参数。
    (void)argument;

    // 等系统和外设先稳定一下，再进入采样流程。
    osDelay(HEART_RATE_TASK_STARTUP_DELAY_MS);

    // 初始化 UART 双缓冲串流模块。
    PpgUartStream_Init(&g_ppg_uart_stream);
    // 初始化 PPG 信号处理器状态。
    PpgSignalProcessor_Init(&g_ppg_signal_processor);
    // 注册蓝牙 DMA 完成/错误回调。
    HeartRate_Task_RegisterBluetoothHooks();

    // 如果存在 EM7028 初始化入口，就先做一次初始化。
    if(HwAccess.em7028.init != NULL) {
        (void)HwAccess.em7028.init();
        // 初始化后顺手记录一次寄存器诊断信息。
        HeartRate_Task_RecordInitDiagnostics();
    }

    // 任务主循环永久运行。
    for(;;) {
        // 这里只看心率监测是否处于“运行态”。
        // 页面或上层逻辑调用 start()/stop() 后，is_running() 会反映结果。
        if((HwAccess.em7028.is_running == NULL) ||
           (HwAccess.em7028.is_running() == 0U)) {
            // 如果之前处于活跃采样态，而现在变成未运行，说明刚停止采样。
            if(stream_active != 0U) {
                // 先请求把剩余 CSV 全部发完。
                PpgUartStream_RequestFlush(&g_ppg_uart_stream);
                // 记录等待开始时刻。
                stop_wait_tick = xTaskGetTickCount();

                // 等待串流彻底空闲，确保前面的 CSV 都真正发出去了。
                while(PpgUartStream_IsIdle(&g_ppg_uart_stream) == 0U) {
                    osDelay(HEART_RATE_STREAM_DRAIN_WAIT_MS);
                    if((xTaskGetTickCount() - stop_wait_tick) >=
                       pdMS_TO_TICKS(HEART_RATE_STREAM_DRAIN_TIMEOUT_MS)) {
                        break;
                    }
                }

                // 在 CSV 排空后补发一次统计。
                HeartRate_Task_SendStatsReport();
                // 清掉活跃态标志，表示下一次 start() 要重新初始化串流状态。
                stream_active = 0U;
            }

            // 心率监测未运行时，不做采样，只低频轮询等待。
            osDelay(HEART_RATE_TASK_IDLE_PERIOD_MS);
            continue;
        }

        // 第一次进入运行态时，需要把串流和处理器状态都重置一遍。
        if(stream_active == 0U) {
            // 清空双缓冲与串流统计，避免沿用上一次监测残留。
            PpgUartStream_Reset(&g_ppg_uart_stream);
            // 清空基线、平滑窗口、上一个有效样本等处理器状态。
            PpgSignalProcessor_Init(&g_ppg_signal_processor);
            // 记录当前 tick，作为 vTaskDelayUntil 的基准点。
            last_wake_tick = xTaskGetTickCount();
            // 标记串流已经进入活跃态。
            stream_active = 1U;
            // 重新开始监测时，再记录一次寄存器读回值。
            HeartRate_Task_RecordInitDiagnostics();
        }

        // 用固定节拍等待下一个采样时刻，保证平均周期稳定在 25ms。
        vTaskDelayUntil(&last_wake_tick, pdMS_TO_TICKS(HEART_RATE_TASK_SAMPLE_PERIOD_MS));

        // 读取当前毫秒时间戳，作为 CSV 的时间轴。
        timestamp_ms = HAL_GetTick();
        // 每个固定采样周期都先占一个 sequence，即使这次读失败也不能跳号。
        sequence = PpgUartStream_BeginSample(&g_ppg_uart_stream, timestamp_ms);
        // 每个周期开始时，先把标志位清零。
        flags = PPG_SAMPLE_FLAG_NONE;

        // 如果没有原始读取接口，就只能把当前周期记成 I2C 错误样本。
        if(HwAccess.em7028.read_raw == NULL) {
            // 记录最近一次读取错误信息。
            HeartRate_Task_RecordReadError();
            // 生成一个无效样本，raw/baseline/ac/filtered 都会输出为 0。
            PpgSignalProcessor_ProcessInvalidSample(&g_ppg_signal_processor,
                                                    sequence,
                                                    timestamp_ms,
                                                    PPG_SAMPLE_FLAG_I2C_ERROR,
                                                    &processed_sample);
            // 把这个无效样本记入统计。
            PpgUartStream_RecordProcessedSample(&g_ppg_uart_stream, &processed_sample);
            // 即使无效，也必须输出一条 CSV，保证时间轴连续。
            (void)PpgUartStream_PushSample(&g_ppg_uart_stream, &processed_sample);
            continue;
        }

        // 第一次尝试读取当前周期的原始 PPG。
        ret = HwAccess.em7028.read_raw(&raw_ppg);
        // 如果第一次读取失败，则当前周期按无效样本处理。
        if(ret != 0) {
            // 记录 I2C/HAL 错误统计。
            HeartRate_Task_RecordReadError();
            // 给当前样本打上 I2C_ERROR 标志。
            flags |= PPG_SAMPLE_FLAG_I2C_ERROR;
            // 生成无效样本记录。
            PpgSignalProcessor_ProcessInvalidSample(&g_ppg_signal_processor,
                                                    sequence,
                                                    timestamp_ms,
                                                    flags,
                                                    &processed_sample);
            // 记录样本统计。
            PpgUartStream_RecordProcessedSample(&g_ppg_uart_stream, &processed_sample);
            // 输出一条 invalid CSV。
            (void)PpgUartStream_PushSample(&g_ppg_uart_stream, &processed_sample);
            continue;
        }

        // 如果当前 raw_ppg 相对“上一个有效样本”不属于明显跳变，就直接作为正常样本处理。
        if(PpgSignalProcessor_IsGrossOutlier(&g_ppg_signal_processor, raw_ppg) == 0U) {
            // 进入信号处理器后，会完成 baseline / ac / filtered 计算。
            PpgSignalProcessor_ProcessValidSample(&g_ppg_signal_processor,
                                                  sequence,
                                                  timestamp_ms,
                                                  raw_ppg,
                                                  flags,
                                                  &processed_sample);
            // 把处理后的有效样本写入统计。
            PpgUartStream_RecordProcessedSample(&g_ppg_uart_stream, &processed_sample);
            // 输出一条完整 CSV。
            (void)PpgUartStream_PushSample(&g_ppg_uart_stream, &processed_sample);
            continue;
        }

        // 走到这里说明第一次读取值出现了“相邻有效样本大跳变”。
        // 先把这次事件记为 gross outlier。
        PpgUartStream_RecordGrossOutlier(&g_ppg_uart_stream);
        // 同步更新处理器内部对应计数。
        g_ppg_signal_processor.gross_outlier_count++;
        // 标记当前样本已经使用过单次重读机会。
        flags |= PPG_SAMPLE_FLAG_RETRY_USED;
        // 立即进行且只允许进行一次重读。
        retry_ret = HwAccess.em7028.read_raw(&retry_raw_ppg);

        // 如果重读成功，并且重读值已经不再是异常跳变，则当前周期仍按有效样本处理。
        if((retry_ret == 0) &&
           (PpgSignalProcessor_IsGrossOutlier(&g_ppg_signal_processor, retry_raw_ppg) == 0U)) {
            // 记录“重读纠正成功”统计。
            PpgUartStream_RecordRetrySuccess(&g_ppg_uart_stream);
            // 同步更新处理器内部成功计数。
            g_ppg_signal_processor.retry_success_count++;
            // 用“重读后的值”进入正常处理流程，同时保留 RETRY_USED 标志。
            PpgSignalProcessor_ProcessValidSample(&g_ppg_signal_processor,
                                                  sequence,
                                                  timestamp_ms,
                                                  retry_raw_ppg,
                                                  flags,
                                                  &processed_sample);
            // 写入样本统计。
            PpgUartStream_RecordProcessedSample(&g_ppg_uart_stream, &processed_sample);
            // 输出一条 valid CSV。
            (void)PpgUartStream_PushSample(&g_ppg_uart_stream, &processed_sample);
            continue;
        }

        // 如果重读本身也失败了，则额外补记一次 I2C 错误。
        if(retry_ret != 0) {
            HeartRate_Task_RecordReadError();
            // 同时把 I2C_ERROR 也体现在当前样本 flags 里。
            flags |= PPG_SAMPLE_FLAG_I2C_ERROR;
        }

        // 走到这里说明：
        // 1. 第一次读取是明显异常；
        // 2. 且唯一允许的一次重读没有把样本纠正回来。
        // 因此当前周期必须作为无效样本丢弃，绝不能写入 baseline 和 smooth buffer。
        PpgUartStream_RecordRetryFail(&g_ppg_uart_stream);
        // 同步更新处理器内部失败计数。
        g_ppg_signal_processor.retry_fail_count++;
        // 给当前样本补上异常跳变标志。
        flags |= PPG_SAMPLE_FLAG_GROSS_OUTLIER;
        // 生成无效样本记录。
        PpgSignalProcessor_ProcessInvalidSample(&g_ppg_signal_processor,
                                                sequence,
                                                timestamp_ms,
                                                flags,
                                                &processed_sample);
        // 记录样本统计。
        PpgUartStream_RecordProcessedSample(&g_ppg_uart_stream, &processed_sample);
        // 仍然输出一条 invalid CSV，保证 sequence 和时间轴连续。
        (void)PpgUartStream_PushSample(&g_ppg_uart_stream, &processed_sample);
    }
}

/**
 * @brief 心率 UART DMA 发送任务。
 *
 * 可以把它理解为“串流后台搬运工”：
 * - 采样任务只负责把完整 CSV 行压入双缓冲；
 * - 这个任务只负责观察 READY 缓冲块，并在合适时机启动 UART DMA；
 * - DMA 完成/错误中断只做最小通知；
 * - 真正的状态推进仍放在任务上下文中完成，避免 ISR 里做复杂逻辑。
 *
 * @param argument 任务参数，当前未使用。
 */
void HeartRate_UartTx_Task(void *argument)
{
    // 当前任务不使用 argument 参数。
    (void)argument;

    // 把“当前发送任务句柄”告诉串流模块，方便 ISR 用任务通知把它唤醒。
    PpgUartStream_BindTxTask(&g_ppg_uart_stream, xTaskGetCurrentTaskHandle());

    // 持续运行，专门负责推进双缓冲发送状态机。
    for(;;) {
        // 等待 ISR 通知或超时醒来。
        (void)ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(HEART_RATE_TX_TASK_WAIT_MS));
        // 在任务上下文里统一处理 DMA 完成、错误恢复和下一块 READY 数据发送。
        PpgUartStream_Process(&g_ppg_uart_stream);
    }
}
