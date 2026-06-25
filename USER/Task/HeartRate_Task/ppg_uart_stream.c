#include "ppg_uart_stream.h"

#include <stdio.h>
#include <string.h>

#include "hwaccess.h"

#define PPG_UART_STREAM_LINE_BUFFER_SIZE   96U

static void PpgUartStream_ClearBuffer(PpgUartStream_t *stream, uint8_t index);
static uint8_t PpgUartStream_FindBufferByState(const PpgUartStream_t *stream,
                                               PpgUartBufferState_t state);
static uint8_t PpgUartStream_FindFreeBuffer(const PpgUartStream_t *stream);
static int PpgUartStream_StartDmaSend(PpgUartStream_t *stream, uint8_t buffer_index);
static void PpgUartStream_NotifyTxTask(PpgUartStream_t *stream);
static void PpgUartStream_NotifyTxTaskFromIsr(PpgUartStream_t *stream);
static int PpgUartStream_PrepareFlushLocked(PpgUartStream_t *stream);
static int PpgUartStream_PushBytes(PpgUartStream_t *stream,
                                   const uint8_t *data,
                                   uint16_t length,
                                   uint16_t sample_count_delta);

void PpgUartStream_Init(PpgUartStream_t *stream)
{
    TaskHandle_t tx_task_handle;

    if(stream == NULL) {
        return;
    }

    tx_task_handle = stream->tx_task_handle;
    memset(stream, 0, sizeof(*stream));
    stream->tx_task_handle = tx_task_handle;
    PpgUartStream_Reset(stream);
    stream->initialized = 1U;
}

void PpgUartStream_Reset(PpgUartStream_t *stream)
{
    uint32_t i;
    TaskHandle_t tx_task_handle;

    if(stream == NULL) {
        return;
    }

    taskENTER_CRITICAL();

    tx_task_handle = stream->tx_task_handle;
    memset(stream, 0, sizeof(*stream));
    stream->tx_task_handle = tx_task_handle;

    for(i = 0U; i < PPG_UART_STREAM_BUFFER_COUNT; i++) {
        PpgUartStream_ClearBuffer(stream, (uint8_t)i);
    }

    stream->filling_index = 0U;
    stream->buffers[stream->filling_index].state = PPG_UART_BUFFER_STATE_FILLING;
    stream->sending_index = PPG_UART_STREAM_INVALID_INDEX;
    stream->initialized = 1U;

    taskEXIT_CRITICAL();
}

void PpgUartStream_BindTxTask(PpgUartStream_t *stream, TaskHandle_t task_handle)
{
    if(stream == NULL) {
        return;
    }

    taskENTER_CRITICAL();
    stream->tx_task_handle = task_handle;
    taskEXIT_CRITICAL();
}

void PpgUartStream_RecordI2cError(PpgUartStream_t *stream,
                                  HAL_StatusTypeDef hal_status,
                                  uint32_t hal_error)
{
    (void)hal_error;

    if(stream == NULL) {
        return;
    }

    taskENTER_CRITICAL();
    stream->stats.sample_i2c_error_count++;
    if(hal_status == HAL_BUSY) {
        stream->stats.i2c_busy_count++;
    } else if(hal_status == HAL_TIMEOUT) {
        stream->stats.i2c_timeout_count++;
    } else {
        stream->stats.i2c_hal_error_count++;
    }
    taskEXIT_CRITICAL();
}

uint32_t PpgUartStream_BeginSample(PpgUartStream_t *stream,
                                   uint32_t timestamp_ms)
{
    uint32_t sequence;
    uint32_t interval_ms;

    if(stream == NULL) {
        return 0U;
    }

    taskENTER_CRITICAL();

    sequence = stream->next_sequence;
    stream->next_sequence++;

    if(stream->has_last_sample_timestamp != 0U) {
        interval_ms = timestamp_ms - stream->last_sample_timestamp_ms;
        if(interval_ms > stream->stats.max_timestamp_interval_ms) {
            stream->stats.max_timestamp_interval_ms = interval_ms;
        }
    }

    stream->last_sample_timestamp_ms = timestamp_ms;
    stream->has_last_sample_timestamp = 1U;

    taskEXIT_CRITICAL();

    return sequence;
}

void PpgUartStream_RecordProcessedSample(PpgUartStream_t *stream,
                                         const PpgProcessedSample_t *sample)
{
    uint32_t same_run_length;

    if((stream == NULL) || (sample == NULL)) {
        return;
    }

    taskENTER_CRITICAL();

    if(sample->valid != 0U) {
        stream->stats.sample_ok_count++;
        stream->stats.valid_processed_count++;
        stream->stats.latest_baseline_ppg = sample->baseline_ppg;
        stream->stats.latest_ac_ppg = sample->ac_ppg;
        stream->stats.latest_filtered_ppg = sample->filtered_ppg;

        if(stream->has_last_raw_ppg != 0U) {
            if(sample->raw_ppg == stream->last_raw_ppg) {
                stream->stats.repeated_raw_count++;
                same_run_length = stream->current_same_raw_run_length + 1U;
            } else {
                same_run_length = 1U;
            }
        } else {
            same_run_length = 1U;
        }

        stream->current_same_raw_run_length = same_run_length;
        if(same_run_length > stream->stats.max_same_raw_run_length) {
            stream->stats.max_same_raw_run_length = same_run_length;
        }

        stream->last_raw_ppg = sample->raw_ppg;
        stream->has_last_raw_ppg = 1U;
    } else {
        stream->stats.invalid_processed_count++;
    }

    taskEXIT_CRITICAL();
}

void PpgUartStream_RecordGrossOutlier(PpgUartStream_t *stream)
{
    if(stream == NULL) {
        return;
    }

    taskENTER_CRITICAL();
    stream->stats.gross_outlier_count++;
    taskEXIT_CRITICAL();
}

void PpgUartStream_RecordRetrySuccess(PpgUartStream_t *stream)
{
    if(stream == NULL) {
        return;
    }

    taskENTER_CRITICAL();
    stream->stats.retry_success_count++;
    taskEXIT_CRITICAL();
}

void PpgUartStream_RecordRetryFail(PpgUartStream_t *stream)
{
    if(stream == NULL) {
        return;
    }

    taskENTER_CRITICAL();
    stream->stats.retry_fail_count++;
    taskEXIT_CRITICAL();
}

void PpgUartStream_RecordHrs1Ctrl(PpgUartStream_t *stream,
                                  uint8_t expected,
                                  uint8_t readback)
{
    if(stream == NULL) {
        return;
    }

    taskENTER_CRITICAL();
    stream->stats.hrs1_ctrl_expected = expected;
    stream->stats.hrs1_ctrl_readback = readback;
    if(expected != readback) {
        stream->stats.hrs1_ctrl_mismatch_count++;
    }
    taskEXIT_CRITICAL();
}

int PpgUartStream_PushSample(PpgUartStream_t *stream,
                             const PpgProcessedSample_t *sample)
{
    char line_buffer[PPG_UART_STREAM_LINE_BUFFER_SIZE];
    int format_length;

    if((stream == NULL) || (sample == NULL)) {
        return -1;
    }

    format_length = snprintf(line_buffer,
                             sizeof(line_buffer),
                             "%lu,%lu,%u,%ld,%ld,%ld,%u,%u\r\n",
                             (unsigned long)sample->sequence,
                             (unsigned long)sample->timestamp_ms,
                             (unsigned int)sample->raw_ppg,
                             (long)sample->baseline_ppg,
                             (long)sample->ac_ppg,
                             (long)sample->filtered_ppg,
                             (unsigned int)sample->valid,
                             (unsigned int)sample->flags);
    if((format_length <= 0) || ((uint32_t)format_length >= sizeof(line_buffer))) {
        return -2;
    }

    return PpgUartStream_PushText(stream, line_buffer, (uint16_t)format_length);
}

int PpgUartStream_PushText(PpgUartStream_t *stream,
                           const char *text,
                           uint16_t length)
{
    if((stream == NULL) || (text == NULL) || (length == 0U)) {
        return -1;
    }

    return PpgUartStream_PushBytes(stream,
                                   (const uint8_t *)text,
                                   length,
                                   1U);
}

void PpgUartStream_RequestFlush(PpgUartStream_t *stream)
{
    if(stream == NULL) {
        return;
    }

    taskENTER_CRITICAL();
    stream->flush_pending = 1U;
    taskEXIT_CRITICAL();

    PpgUartStream_NotifyTxTask(stream);
}

void PpgUartStream_Process(PpgUartStream_t *stream)
{
    uint8_t buffer_index;
    int send_ret;

    if((stream == NULL) || (stream->initialized == 0U)) {
        return;
    }

    taskENTER_CRITICAL();

    if(stream->dma_complete_pending != 0U) {
        buffer_index = PpgUartStream_FindBufferByState(stream, PPG_UART_BUFFER_STATE_SENDING);
        if(buffer_index < PPG_UART_STREAM_BUFFER_COUNT) {
            stream->stats.sent_samples += stream->buffers[buffer_index].sample_count;
            stream->stats.dma_complete_count++;
            PpgUartStream_ClearBuffer(stream, buffer_index);
        }
        stream->sending_index = PPG_UART_STREAM_INVALID_INDEX;
        stream->dma_busy = 0U;
        stream->dma_complete_pending = 0U;
    }

    if(stream->dma_error_pending != 0U) {
        buffer_index = PpgUartStream_FindBufferByState(stream, PPG_UART_BUFFER_STATE_SENDING);
        if(buffer_index < PPG_UART_STREAM_BUFFER_COUNT) {
            stream->buffers[buffer_index].state = PPG_UART_BUFFER_STATE_READY;
        }
        stream->sending_index = PPG_UART_STREAM_INVALID_INDEX;
        stream->dma_busy = 0U;
        stream->dma_error_pending = 0U;
    }

    if(stream->flush_pending != 0U) {
        if(PpgUartStream_PrepareFlushLocked(stream) == 0) {
            stream->flush_pending = 0U;
        }
    }

    if(stream->dma_busy != 0U) {
        taskEXIT_CRITICAL();
        return;
    }

    buffer_index = PpgUartStream_FindBufferByState(stream, PPG_UART_BUFFER_STATE_READY);
    taskEXIT_CRITICAL();

    if(buffer_index >= PPG_UART_STREAM_BUFFER_COUNT) {
        return;
    }

    send_ret = PpgUartStream_StartDmaSend(stream, buffer_index);
    if(send_ret != 0) {
        PpgUartStream_NotifyTxTask(stream);
    }
}

void PpgUartStream_OnTxCompleteFromIsr(void *context)
{
    PpgUartStream_t *stream = (PpgUartStream_t *)context;
    UBaseType_t irq_state;

    if(stream == NULL) {
        return;
    }

    irq_state = taskENTER_CRITICAL_FROM_ISR();
    stream->dma_complete_pending = 1U;
    taskEXIT_CRITICAL_FROM_ISR(irq_state);

    PpgUartStream_NotifyTxTaskFromIsr(stream);
}

void PpgUartStream_OnUartErrorFromIsr(void *context)
{
    PpgUartStream_t *stream = (PpgUartStream_t *)context;
    UBaseType_t irq_state;

    if(stream == NULL) {
        return;
    }

    irq_state = taskENTER_CRITICAL_FROM_ISR();
    stream->stats.dma_error_count++;
    stream->dma_error_pending = 1U;
    taskEXIT_CRITICAL_FROM_ISR(irq_state);

    PpgUartStream_NotifyTxTaskFromIsr(stream);
}

uint8_t PpgUartStream_IsIdle(const PpgUartStream_t *stream)
{
    uint8_t i;

    if((stream == NULL) || (stream->initialized == 0U)) {
        return 1U;
    }

    if((stream->dma_busy != 0U) ||
       (stream->dma_complete_pending != 0U) ||
       (stream->dma_error_pending != 0U) ||
       (stream->flush_pending != 0U)) {
        return 0U;
    }

    for(i = 0U; i < PPG_UART_STREAM_BUFFER_COUNT; i++) {
        if((stream->buffers[i].state == PPG_UART_BUFFER_STATE_READY) ||
           (stream->buffers[i].state == PPG_UART_BUFFER_STATE_SENDING) ||
           ((stream->buffers[i].state == PPG_UART_BUFFER_STATE_FILLING) &&
            (stream->buffers[i].length > 0U))) {
            return 0U;
        }
    }

    return 1U;
}

void PpgUartStream_GetStats(const PpgUartStream_t *stream,
                            PpgUartStreamStats_t *stats)
{
    if((stream == NULL) || (stats == NULL)) {
        return;
    }

    taskENTER_CRITICAL();
    *stats = stream->stats;
    taskEXIT_CRITICAL();
}

static void PpgUartStream_ClearBuffer(PpgUartStream_t *stream, uint8_t index)
{
    stream->buffers[index].length = 0U;
    stream->buffers[index].sample_count = 0U;
    stream->buffers[index].state = PPG_UART_BUFFER_STATE_FREE;
}

static uint8_t PpgUartStream_FindBufferByState(const PpgUartStream_t *stream,
                                               PpgUartBufferState_t state)
{
    uint8_t i;

    for(i = 0U; i < PPG_UART_STREAM_BUFFER_COUNT; i++) {
        if(stream->buffers[i].state == state) {
            return i;
        }
    }

    return PPG_UART_STREAM_INVALID_INDEX;
}

static uint8_t PpgUartStream_FindFreeBuffer(const PpgUartStream_t *stream)
{
    return PpgUartStream_FindBufferByState(stream, PPG_UART_BUFFER_STATE_FREE);
}

static int PpgUartStream_StartDmaSend(PpgUartStream_t *stream, uint8_t buffer_index)
{
    uint16_t length;
    const uint8_t *data;
    int send_ret;

    if((stream == NULL) || (buffer_index >= PPG_UART_STREAM_BUFFER_COUNT)) {
        return -1;
    }

    if(HwAccess.bluetooth.send_dma == NULL) {
        taskENTER_CRITICAL();
        stream->stats.dma_error_count++;
        taskEXIT_CRITICAL();
        return -2;
    }

    taskENTER_CRITICAL();

    if((stream->dma_busy != 0U) ||
       (stream->buffers[buffer_index].state != PPG_UART_BUFFER_STATE_READY) ||
       (stream->buffers[buffer_index].length == 0U)) {
        taskEXIT_CRITICAL();
        return -3;
    }

    stream->buffers[buffer_index].state = PPG_UART_BUFFER_STATE_SENDING;
    stream->sending_index = buffer_index;
    stream->dma_busy = 1U;
    data = stream->buffers[buffer_index].data;
    length = stream->buffers[buffer_index].length;

    taskEXIT_CRITICAL();

    send_ret = HwAccess.bluetooth.send_dma(data, length);
    if(send_ret == 0) {
        taskENTER_CRITICAL();
        stream->stats.dma_start_count++;
        taskEXIT_CRITICAL();
        return 0;
    }

    taskENTER_CRITICAL();
    if(stream->buffers[buffer_index].state == PPG_UART_BUFFER_STATE_SENDING) {
        stream->buffers[buffer_index].state = PPG_UART_BUFFER_STATE_READY;
    }
    stream->sending_index = PPG_UART_STREAM_INVALID_INDEX;
    stream->dma_busy = 0U;
    stream->stats.dma_error_count++;
    taskEXIT_CRITICAL();

    return -4;
}

static void PpgUartStream_NotifyTxTask(PpgUartStream_t *stream)
{
    TaskHandle_t task_handle;

    taskENTER_CRITICAL();
    task_handle = stream->tx_task_handle;
    taskEXIT_CRITICAL();

    if(task_handle != NULL) {
        xTaskNotifyGive(task_handle);
    }
}

static void PpgUartStream_NotifyTxTaskFromIsr(PpgUartStream_t *stream)
{
    if(stream->tx_task_handle != NULL) {
        vTaskNotifyGiveFromISR(stream->tx_task_handle, NULL);
    }
}

static int PpgUartStream_PrepareFlushLocked(PpgUartStream_t *stream)
{
    uint8_t buffer_index;
    uint8_t free_index;

    buffer_index = stream->filling_index;
    if(buffer_index >= PPG_UART_STREAM_BUFFER_COUNT) {
        return 0;
    }

    if(stream->buffers[buffer_index].state != PPG_UART_BUFFER_STATE_FILLING) {
        return 0;
    }

    if(stream->buffers[buffer_index].length == 0U) {
        return 0;
    }

    stream->buffers[buffer_index].state = PPG_UART_BUFFER_STATE_READY;

    free_index = PpgUartStream_FindFreeBuffer(stream);
    if(free_index < PPG_UART_STREAM_BUFFER_COUNT) {
        stream->filling_index = free_index;
        stream->buffers[free_index].state = PPG_UART_BUFFER_STATE_FILLING;
    } else {
        stream->filling_index = PPG_UART_STREAM_INVALID_INDEX;
    }

    PpgUartStream_NotifyTxTask(stream);
    return 0;
}

static int PpgUartStream_PushBytes(PpgUartStream_t *stream,
                                   const uint8_t *data,
                                   uint16_t length,
                                   uint16_t sample_count_delta)
{
    uint8_t buffer_index;
    uint8_t free_index;
    PpgUartBuffer_t *buffer;

    taskENTER_CRITICAL();

    buffer_index = stream->filling_index;
    if(buffer_index >= PPG_UART_STREAM_BUFFER_COUNT) {
        free_index = PpgUartStream_FindFreeBuffer(stream);
        if(free_index >= PPG_UART_STREAM_BUFFER_COUNT) {
            stream->stats.dropped_samples++;
            taskEXIT_CRITICAL();
            return -2;
        }

        stream->filling_index = free_index;
        stream->buffers[free_index].state = PPG_UART_BUFFER_STATE_FILLING;
        buffer_index = free_index;
    }

    buffer = &stream->buffers[buffer_index];
    if(buffer->state != PPG_UART_BUFFER_STATE_FILLING) {
        stream->stats.dropped_samples++;
        taskEXIT_CRITICAL();
        return -3;
    }

    if(((uint32_t)buffer->length + (uint32_t)length) > PPG_UART_STREAM_BUFFER_SIZE) {
        buffer->state = PPG_UART_BUFFER_STATE_READY;
        PpgUartStream_NotifyTxTask(stream);

        free_index = PpgUartStream_FindFreeBuffer(stream);
        if(free_index >= PPG_UART_STREAM_BUFFER_COUNT) {
            stream->filling_index = PPG_UART_STREAM_INVALID_INDEX;
            stream->stats.dropped_samples++;
            taskEXIT_CRITICAL();
            return -4;
        }

        stream->filling_index = free_index;
        stream->buffers[free_index].state = PPG_UART_BUFFER_STATE_FILLING;
        buffer_index = free_index;
        buffer = &stream->buffers[buffer_index];
    }

    memcpy(&buffer->data[buffer->length], data, length);
    buffer->length = (uint16_t)(buffer->length + length);
    buffer->sample_count = (uint16_t)(buffer->sample_count + sample_count_delta);
    stream->stats.pushed_samples += (uint32_t)sample_count_delta;

    taskEXIT_CRITICAL();

    return 0;
}
