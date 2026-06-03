#include "lvgl.h"
#include "cmsis_os2.h"
#include "hwaccess.h"
#include "st7789v.h"

#define LV_PORT_DISP_BUFFER_LINES    40U
#define LV_PORT_DISP_Y_OFFSET        20U
#define LV_PORT_DISP_DMA_READY_FLAG  0x00000001U
#define LV_PORT_DISP_DMA_TIMEOUT_MS  100U

static uint8_t disp_buf_1[LCD_WIDTH * LV_PORT_DISP_BUFFER_LINES * 2U];
static uint8_t disp_buf_2[LCD_WIDTH * LV_PORT_DISP_BUFFER_LINES * 2U];

static osThreadId_t lvgl_thread_id;

static void disp_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map);
static void disp_flush_wait_cb(lv_display_t *disp);

/**
 * @brief 初始化 LVGL 显示移植层。
 */
void lv_port_disp_init(void)
{
    lv_display_t *disp;

    lvgl_thread_id = osThreadGetId();
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
    if (lvgl_thread_id != NULL) {
        (void)osThreadFlagsSet(lvgl_thread_id, LV_PORT_DISP_DMA_READY_FLAG);
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

    /* LCD 物理有效显示区域比 LVGL 原点向下偏移 20 像素。 */
    (void)osThreadFlagsClear(LV_PORT_DISP_DMA_READY_FLAG);
    st7789_SetWindow(x1, y1, x2, y2);
    st7789_WritePixels(px_map, width * height * 2U);
#if (ST7789_FILL_MODE != ST7789_FILL_MODE_DMA)
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
    (void)osThreadFlagsWait(LV_PORT_DISP_DMA_READY_FLAG,
                            osFlagsWaitAny,
                            LV_PORT_DISP_DMA_TIMEOUT_MS);
    lv_display_flush_ready(disp);
#else
    (void)disp;
#endif
}
