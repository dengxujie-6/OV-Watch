#include "lvgl.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "stm32f4xx_hal.h"
#include "hwaccess.h"
#include "st7789v.h"

#define LV_PORT_DISP_BUFFER_LINES    (LCD_HEIGHT / 10U)
#define LV_PORT_DISP_Y_OFFSET        20U
#define LV_PORT_DISP_DMA_TIMEOUT_MS  100U

static uint8_t disp_buf_1[LCD_WIDTH * LV_PORT_DISP_BUFFER_LINES * 2U];
static uint8_t disp_buf_2[LCD_WIDTH * LV_PORT_DISP_BUFFER_LINES * 2U];

static StaticSemaphore_t dma_ready_sem_buffer;
static SemaphoreHandle_t dma_ready_sem;

/**
 * @brief LCD flush / DMA 调试变量。
 *
 * 这些变量用于 Keil Watch 观察 LVGL 刷新链路是否仍在推进，
 * 不参与正式功能逻辑。
 */
volatile uint32_t g_lvgl_disp_flush_request_count;
volatile uint32_t g_lvgl_disp_flush_ready_count;
volatile uint32_t g_lvgl_disp_flush_wait_count;
volatile uint32_t g_lvgl_disp_flush_wait_timeout_count;
volatile uint32_t g_lvgl_disp_dma_callback_count;
volatile uint32_t g_lvgl_disp_last_flush_tick_ms;
volatile uint32_t g_lvgl_disp_last_flush_ready_tick_ms;
volatile uint32_t g_lvgl_disp_last_wait_tick_ms;
volatile uint32_t g_lvgl_disp_last_wait_result;
volatile uint32_t g_lvgl_disp_last_px_bytes;
volatile int32_t g_lvgl_disp_last_area_x1;
volatile int32_t g_lvgl_disp_last_area_y1;
volatile int32_t g_lvgl_disp_last_area_x2;
volatile int32_t g_lvgl_disp_last_area_y2;

static void disp_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map);
static void disp_flush_wait_cb(lv_display_t *disp);

/**
 * @brief 初始化 LVGL 显示移植层。
 */
void lv_port_disp_init(void)
{
    lv_display_t *disp;

    dma_ready_sem = xSemaphoreCreateBinaryStatic(&dma_ready_sem_buffer);
    disp = lv_display_create(LCD_WIDTH, LCD_HEIGHT);

    lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565_SWAPPED);
    lv_display_set_buffers(disp,
                           disp_buf_1,
                           disp_buf_2,
                           sizeof(disp_buf_1),
                           LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(disp, disp_flush_cb);
    lv_display_set_flush_wait_cb(disp, disp_flush_wait_cb);
}

/**
 * @brief LCD 像素 DMA 发送完成回调。
 *
 * 该函数由 ST7789 BSP 在中断上下文触发，只通知 LVGL 任务，不直接调用 LVGL API。
 */
void st7789_TxCpltCallback(void)
{
    // DMA 完成回调若持续增长，说明 LCD SPI/DMA 中断链路仍然活着。
    g_lvgl_disp_dma_callback_count++;

    if (dma_ready_sem != NULL) {
        if (xPortIsInsideInterrupt() != pdFALSE) {
            BaseType_t higher_priority_task_woken = pdFALSE;

            (void)xSemaphoreGiveFromISR(dma_ready_sem, &higher_priority_task_woken);
            portYIELD_FROM_ISR(higher_priority_task_woken);
        } else {
            (void)xSemaphoreGive(dma_ready_sem);
        }
    }
}

/**
 * @brief 将 LVGL 渲染完成的像素刷新到 LCD。
 *
 * @param disp LVGL 显示对象。
 * @param area 需要更新的 LVGL 无效区域。
 * @param px_map LVGL 渲染生成的 RGB565 像素缓冲区。
 */
static void disp_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    uint16_t x1 = (uint16_t)area->x1;
    uint16_t y1 = (uint16_t)(area->y1 + LV_PORT_DISP_Y_OFFSET);
    uint16_t x2 = (uint16_t)area->x2;
    uint16_t y2 = (uint16_t)(area->y2 + LV_PORT_DISP_Y_OFFSET);
    uint32_t width = (uint32_t)(x2 - x1 + 1U);
    uint32_t height = (uint32_t)(y2 - y1 + 1U);
    uint32_t px_bytes = width * height * 2U;

    // 记录最近一次 flush 请求，卡顿时可比对请求数是否还在增长。
    g_lvgl_disp_flush_request_count++;
    g_lvgl_disp_last_flush_tick_ms = HAL_GetTick();
    g_lvgl_disp_last_area_x1 = area->x1;
    g_lvgl_disp_last_area_y1 = area->y1;
    g_lvgl_disp_last_area_x2 = area->x2;
    g_lvgl_disp_last_area_y2 = area->y2;
    g_lvgl_disp_last_px_bytes = px_bytes;

    // LCD 物理有效显示区域比 LVGL 原点向下偏移 20 像素。
    if (dma_ready_sem != NULL) {
        (void)xSemaphoreTake(dma_ready_sem, 0U);
    }
    st7789_SetWindow(x1, y1, x2, y2);
    st7789_WritePixels(px_map, px_bytes);
#if (ST7789_FILL_MODE != ST7789_FILL_MODE_DMA)
    g_lvgl_disp_flush_ready_count++;
    g_lvgl_disp_last_flush_ready_tick_ms = HAL_GetTick();
    lv_display_flush_ready(disp);
#else
    (void)disp;
#endif
}

/**
 * @brief 等待 LCD DMA 刷新完成。
 *
 * LVGL 会在 lv_timer_handler 内部调用本函数等待异步 flush 结束。
 *
 * @param disp LVGL 显示对象。
 */
static void disp_flush_wait_cb(lv_display_t *disp)
{
#if (ST7789_FILL_MODE == ST7789_FILL_MODE_DMA)
    BaseType_t wait_result = pdFALSE;

    g_lvgl_disp_flush_wait_count++;
    g_lvgl_disp_last_wait_tick_ms = HAL_GetTick();

    if (dma_ready_sem != NULL) {
        wait_result = xSemaphoreTake(dma_ready_sem, pdMS_TO_TICKS(LV_PORT_DISP_DMA_TIMEOUT_MS));
    }

    g_lvgl_disp_last_wait_result = (uint32_t)wait_result;
    if (wait_result != pdTRUE) {
        g_lvgl_disp_flush_wait_timeout_count++;
    }

    g_lvgl_disp_flush_ready_count++;
    g_lvgl_disp_last_flush_ready_tick_ms = HAL_GetTick();
    lv_display_flush_ready(disp);
#else
    (void)disp;
#endif
}
