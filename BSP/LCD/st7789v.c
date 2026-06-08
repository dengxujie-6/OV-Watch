/**
 * @file st7789v.c
 * @author Renato Freitas (freitas-renato@outlook.com)
 * @brief ST7789V LCD 驱动函数实现
 * @version 0.1
 * @date 2021-03-24
 * 
 * @note 本文件沿用 STMicroelectronics Nucleo/Discovery 板卡中的 LCD_IO 接口风格
 * @see 
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "st7789v.h"
#include "spi.h"
#include "tim.h"
#include "stm32f4xx_hal_gpio.h"

/*************************************************************************************************/
/*                      GPIO、SPI、TIM 外设由 CubeMX 或其他初始化流程完成配置                      */
/*************************************************************************************************/
/* LCD 控制引脚。这些 GPIO 引脚在 Core/Src/gpio.c 中完成初始化。*/
#define LCD_DC_GPIO_PORT        GPIOB
#define LCD_DC_GPIO_PIN         GPIO_PIN_9     // PB9：命令/数据选择 LCD_DC
#define LCD_CS_GPIO_PORT        GPIOB
#define LCD_CS_GPIO_PIN         GPIO_PIN_8     // PB8：片选 LCD_CS
#define LCD_RST_GPIO_PORT       GPIOB
#define LCD_RST_GPIO_PIN        GPIO_PIN_7     // PB7：硬件复位 LCD_RST

/* PB0 被配置为 TIM3_CH3 PWM 输出，用于 LCD 背光。*/
#define LCD_BLK_TIM             htim3
#define LCD_BLK_TIM_CHANNEL     TIM_CHANNEL_3

/* PB3/PB5 在 Core/Src/spi.c 中配置为 SPI1_SCK/SPI1_MOSI。*/
#define LCD_SPI                 hspi1
#define LCD_SPI_TIMEOUT_MS      500U

#if ((ST7789_FILL_MODE != ST7789_FILL_MODE_PIXEL) && \
     (ST7789_FILL_MODE != ST7789_FILL_MODE_BUFFER) && \
     (ST7789_FILL_MODE != ST7789_FILL_MODE_DMA))
#error "Invalid ST7789_FILL_MODE"
#endif

static volatile uint8_t lcd_dma_notify_enabled;

/*************************************************************************************************/
/*                      GPIO、SPI、TIM 外设由 CubeMX 或其他初始化流程完成配置                      */
/*************************************************************************************************/


/**
 * @brief ST7789 命令结构体。 */
typedef struct {
    uint8_t command;  // 命令 ID
    uint16_t waitMs;  // 命令发送后的延时时间，单位 ms
    uint8_t dataSize; // 命令参数数据长度
    uint8_t *data;
} st7789_command_t;

/* ST7789 命令/数据写入流程使用的文件内私有函数。*/
static void st7789_RunCommand(const st7789_command_t *command);
static void st7789_RunCommands(const st7789_command_t *sequence);
static void CS_IDLE(void);
static void CS_ACTIVE(void);
static void DC_COMMAND(void);
static void DC_DATA(void);
static void RESX_IDLE(void);
static void RESX_ACTIVE(void);
static void LCD_BLK_ON(void);
static void LCD_IO_Init(void);
static void LCD_IO_WriteData(uint8_t *data, uint8_t length);
#if (ST7789_FILL_MODE == ST7789_FILL_MODE_BUFFER)
static void LCD_IO_WriteDatas(uint8_t *data, uint16_t length);
#elif (ST7789_FILL_MODE == ST7789_FILL_MODE_DMA)
static void LCD_IO_WriteDatasDMA(uint8_t *data, uint16_t length);
static void LCD_IO_WriteDatasDMA_Blocking(uint8_t *data, uint16_t length);
#endif
static void LCD_IO_WriteCommand(uint8_t command);
static void LCD_IO_Delay(uint32_t delay);
static void LCD_IO_WriteDataByte(uint8_t data);


/**
 * @brief ST7789 初始化流程，配置显示接口和显示参数。
 *
 */
void st7789_Init(void) {
    LCD_IO_Init();

    //* CASET 设置显示宽度范围
    const uint8_t caset[4] = {
        0x00, 0x00,
        (ST7789_LCD_WIDTH - 1) >> 8, (ST7789_LCD_WIDTH - 1) & 0xFF
    };

    //* RASET 设置显示高度范围
    const uint8_t raset[4] = {
        0x00, 0x00,
        (ST7789_LCD_HEIGHT - 1) >> 8, (ST7789_LCD_HEIGHT - 1) & 0xFF
    };

    const st7789_command_t initSequence[] = {
        // 进入睡眠
        {ST7789_CMD_SLPIN, 10, 0, NULL},                    // 进入睡眠
        {ST7789_CMD_SWRESET, 200, 0, NULL},                 // 软件复位
        {ST7789_CMD_SLPOUT, 120, 0, NULL},                  // 退出睡眠
        {ST7789_CMD_CMD2EN, 100, 0, NULL},

        {ST7789_CMD_MADCTL, 0, 1, ( uint8_t *)"\x00"},      // 行列地址顺序
        {ST7789_CMD_COLMOD, 0, 1, ( uint8_t *)"\x55"},      // 16 位 RGB 模式

        // //* 添加 VSYNC、HSYNC 配置
        // {ST7789_CMD_RGBCTRL, 0, 3, (uint8_t *)"\x42\x08\x3c"},

        {ST7789_CMD_INVON, 0, 0, NULL},                     // 打开显示反色
        {ST7789_CMD_CASET, 0, 4, ( uint8_t *)caset},        // 设置宽度范围
        {ST7789_CMD_RASET, 0, 4, ( uint8_t *)raset},        // 设置高度范围

        // Porch 参数设置
        {ST7789_CMD_PORCTRL, 0, 5, ( uint8_t *)"\x0c\x0c\x00\x33\x33"},
        // 设置 VGH 为 12.54V，VGL 为 -9.6V
        {ST7789_CMD_GCTRL, 0, 1, ( uint8_t *)"\x35"},
        // 设置 VCOM 为 1.475V
        {ST7789_CMD_VCOMS, 0, 1, ( uint8_t *)"\x1f"},
        // 使能 VDV/VRH 控制
        {ST7789_CMD_VDVVRHEN, 0, 1, ( uint8_t *)"\x01"},

        // LCM 控制
        {ST7789_CMD_LCMCTRL, 0, 1, ( uint8_t *)"\x2c"},
        // VAP(GVDD) 电压计算：4.45 + (VCOM + VCOM 偏移 + VDV)
        {ST7789_CMD_VRHS, 0, 1, ( uint8_t *)"\x12"},
        // 设置 VDV 为 0V
        {ST7789_CMD_VDVSET, 0, 1, ( uint8_t *)"\x20"},
        // 设置 AVDD 为 6.8V，AVCL 为 -4.8V，VDDS 为 2.3V
        {ST7789_CMD_PWCTRL1, 0, 2, ( uint8_t *)"\xa4\xa1"},
        // 设置刷新率为 60 fps
        {ST7789_CMD_FRCTR2, 0, 1, ( uint8_t *)"\x0f"},
        // 设置 Gamma 为 2.2
        {ST7789_CMD_GAMSET, 0, 1, (uint8_t *)"\x01"},
        // Gamma 曲线
        {ST7789_CMD_PVGAMCTRL, 0, 14, ( uint8_t *)"\xd0\x08\x11\x08\x0c\x15\x39\x33\x50\x36\x13\x14\x29\x2d"},
        {ST7789_CMD_NVGAMCTRL, 0, 14, ( uint8_t *)"\xd0\x08\x10\x08\x06\x06\x39\x44\x51\x0b\x16\x14\x2f\x31"},
        
        {ST7789_CMDLIST_END, 0, 0, NULL}                   // 命令序列结束
    };

    st7789_RunCommands(initSequence);

    LCD_IO_Delay(10);
    st7789_Clear(ST7789_BLACK);

    const st7789_command_t initSequence2[] = {
        {ST7789_CMD_RGBCTRL, 0, 3, (uint8_t *)"\x42\x08\x3c"},  // 设置 HSYNC = 0x3C，VSYNC = 0x80
        {ST7789_CMD_RAMCTRL, 0, 2, (uint8_t*)"\x11\xc2"},       // RAMCTRL 选择 RGB 接口
        {ST7789_CMD_DISPON, 100, 0, NULL},                      // 打开显示
        {ST7789_CMD_SLPOUT, 100, 0, NULL},                      // 退出睡眠
        {ST7789_CMD_RAMWR, 50, 0, NULL},                        // 开始写 GRAM
        {ST7789_CMDLIST_END, 0, 0, NULL},                       // 命令序列结束
    };

    st7789_RunCommands(initSequence2);
}

void st7789_Reset(void) {
    LCD_IO_Init();
}

/**
 * @brief 反初始化 LCD 显示模块。
 *
 * 关闭显示输出并将背光亮度设置为 0。 */
void st7789_DeInit(void)
{
    st7789_DisplayOff();
    st7789_SetBacklight(0U);
}


/**
 * @brief 打开显示。
 *
 */
void st7789_DisplayOn(void) {
    LCD_IO_WriteCommand(ST7789_CMD_DISPON);
}

/**
 * @brief 关闭显示。
 *
 */
void st7789_DisplayOff(void) {
    LCD_IO_WriteCommand(ST7789_CMD_DISPOFF);
}

/**
 * @brief 设置 LCD 背光亮度。
 *
 * @param brightness 背光亮度百分比，范围 0~100。 */
void st7789_SetBacklight(uint8_t brightness)
{
    uint32_t autoReload;
    uint32_t compare;

    if (brightness > 100U) {
        brightness = 100U;
    }

    autoReload = __HAL_TIM_GET_AUTORELOAD(&LCD_BLK_TIM);
    compare = (autoReload * (uint32_t)brightness) / 100U;

    __HAL_TIM_SET_COMPARE(&LCD_BLK_TIM, LCD_BLK_TIM_CHANNEL, compare);

    if (brightness == 0U) {
        (void)HAL_TIM_PWM_Stop(&LCD_BLK_TIM, LCD_BLK_TIM_CHANNEL);
    } else {
        (void)HAL_TIM_PWM_Start(&LCD_BLK_TIM, LCD_BLK_TIM_CHANNEL);
    }
}

/**
 * @brief 通过 SPI 执行一条 ST7789V 命令。
 *
 * @param command 命令结构体指针。 */
static void st7789_RunCommand(const st7789_command_t *command) {
    LCD_IO_WriteCommand(command->command);

    LCD_IO_WriteData(command->data, command->dataSize);

    if (command->waitMs > 0) {
        LCD_IO_Delay(command->waitMs);
    }
}

/**
 * @brief 执行预定义的命令序列。
 *
 * @param sequence 命令序列数组指针
 */
static void st7789_RunCommands(const st7789_command_t *sequence) {
    while (sequence->command != ST7789_CMDLIST_END) {
        st7789_RunCommand(sequence);
        sequence++;
    }
}

/**
 * @brief 设置显示 RAM 写像素窗口。
 *
 * @param xStart  水平方向起始坐标
 * @param yStart  垂直方向起始坐标
 * @param xEnd    水平方向结束坐标
 * @param yEnd    垂直方向结束坐标
 */
void st7789_SetWindow(uint16_t xStart, uint16_t yStart, uint16_t xEnd, uint16_t yEnd) {
    uint8_t caset[4];
    uint8_t raset[4];

    caset[0] = (uint8_t)(xStart >> 8);
    caset[1] = (uint8_t)(xStart & 0xFF);
    caset[2] = (uint8_t)(xEnd >> 8);
    caset[3] = (uint8_t)(xEnd & 0xFF);

    raset[0] = (uint8_t)(yStart >> 8);
    raset[1] = (uint8_t)(yStart & 0xFF);
    raset[2] = (uint8_t)(yEnd >> 8);
    raset[3] = (uint8_t)(yEnd & 0xFF);


    st7789_command_t sequence[] = {
        {ST7789_CMD_CASET, 0, 4, caset},
        {ST7789_CMD_RASET, 0, 4, raset},
        {ST7789_CMDLIST_END, 0, 0, NULL},
    };

    st7789_RunCommands(sequence);

    const st7789_command_t ram_wr = {ST7789_CMD_RAMWR, 0, 0, NULL};
    st7789_RunCommand(&ram_wr);
}

/**
 * @brief Write RGB565 pixel bytes to the active LCD RAM window.
 *
 * @param pixels Pointer to RGB565 pixel data, high byte first.
 * @param length Data length in bytes.
 */
void st7789_WritePixels(const uint8_t *pixels, uint32_t length)
{
#if (ST7789_FILL_MODE == ST7789_FILL_MODE_DMA)
    uint16_t chunk = (length > 65534U) ? 65534U : (uint16_t)length;

    LCD_IO_WriteDatasDMA((uint8_t *)pixels, chunk);
#else
    while (length > 0U) {
#if (ST7789_FILL_MODE == ST7789_FILL_MODE_PIXEL)
        uint8_t chunk = (length > 254U) ? 254U : (uint8_t)length;
        LCD_IO_WriteData((uint8_t *)pixels, chunk);
#else
        uint16_t chunk = (length > 65534U) ? 65534U : (uint16_t)length;
        LCD_IO_WriteDatas((uint8_t *)pixels, chunk);
#endif

        pixels += chunk;
        length -= chunk;
    }
#endif
}

/**
 * @brief 填充矩形区域。
 *
 * @param color     16 位 RGB565 颜色值
 * @param startX    矩形起始 X 坐标
 * @param startY    矩形起始 Y 坐标
 * @param width     矩形宽度
 * @param height    矩形高度
 */
void st7789_FillArea(uint16_t color, uint16_t startX, uint16_t startY, uint16_t width, uint16_t height) {
    uint8_t hi;
    uint8_t lo;
    uint32_t pixelCount;

    if ((width == 0U) || (height == 0U)) {
        return;
    }

    hi = (uint8_t)((color >> 8) & 0xFF);
    lo = (uint8_t)color;
    pixelCount = (uint32_t)width * (uint32_t)height;

    //* 根据起点和宽高设置 LCD 写入窗口
    st7789_SetWindow(startX, startY, startX + width - 1, startY + height - 1);

#if (ST7789_FILL_MODE == ST7789_FILL_MODE_PIXEL)
    uint8_t pixel[2] = {hi, lo};
    while (pixelCount > 0U) {
        LCD_IO_WriteData(pixel, sizeof(pixel));
        pixelCount--;
    }
#else
    static uint8_t fillBuffer[ST7789_FILL_BUFFER_PIXELS * 2U];
    uint32_t index;

    for (index = 0U; index < ST7789_FILL_BUFFER_PIXELS; index++) {
        fillBuffer[index * 2U] = hi;
        fillBuffer[index * 2U + 1U] = lo;
    }

    while (pixelCount > 0U) {
        uint16_t chunkPixels = (pixelCount > ST7789_FILL_BUFFER_PIXELS) ?
                               ST7789_FILL_BUFFER_PIXELS :
                               (uint16_t)pixelCount;
        uint16_t chunkBytes = (uint16_t)(chunkPixels * 2U);

#if (ST7789_FILL_MODE == ST7789_FILL_MODE_BUFFER)
        LCD_IO_WriteDatas(fillBuffer, chunkBytes);
#elif (ST7789_FILL_MODE == ST7789_FILL_MODE_DMA)
        LCD_IO_WriteDatasDMA_Blocking(fillBuffer, chunkBytes);
#endif

        pixelCount -= chunkPixels;
    }
#endif
}

void st7789_Clear(uint16_t color) 
{
    st7789_FillArea(color, 0, 0, ST7789_LCD_WIDTH, ST7789_LCD_HEIGHT+20);
}

/**
 * @brief 将 LCD 片选引脚设置为空闲电平。
 *
 * PB8 连接到 LCD_CS。高电平表示 LCD 不响应当前 SPI 总线传输。 */
static void CS_IDLE(void)
{
    HAL_GPIO_WritePin(LCD_CS_GPIO_PORT, LCD_CS_GPIO_PIN, GPIO_PIN_SET);
}

/**
 * @brief 将 LCD 片选引脚设置为有效电平。
 *
 * PB8 连接到 LCD_CS。低电平表示在 SPI 传输前选中 LCD。 */
static void CS_ACTIVE(void)
{
    HAL_GPIO_WritePin(LCD_CS_GPIO_PORT, LCD_CS_GPIO_PIN, GPIO_PIN_RESET);
}

/**
 * @brief 通过 LCD D/C 引脚选择命令传输。
 *
 * PB9 连接到 LCD_DC。低电平表示接下来的 SPI 字节是命令。 */
static void DC_COMMAND(void)
{
    HAL_GPIO_WritePin(LCD_DC_GPIO_PORT, LCD_DC_GPIO_PIN, GPIO_PIN_RESET);
}

/**
 * @brief 通过 LCD D/C 引脚选择数据传输。
 *
 * PB9 连接到 LCD_DC。高电平表示接下来的 SPI 字节是数据。 */
static void DC_DATA(void)
{
    HAL_GPIO_WritePin(LCD_DC_GPIO_PORT, LCD_DC_GPIO_PIN, GPIO_PIN_SET);
}

/**
 * @brief 释放 LCD 硬件复位引脚。
 *
 * PB7 连接到 LCD_RST。高电平让 ST7789 离开复位状态。 */
static void RESX_IDLE(void)
{
    HAL_GPIO_WritePin(LCD_RST_GPIO_PORT, LCD_RST_GPIO_PIN, GPIO_PIN_SET);
}

/**
 * @brief 拉低 LCD 硬件复位引脚。
 *
 * PB7 连接到 LCD_RST。低电平会复位 ST7789 控制器。 */
static void RESX_ACTIVE(void)
{
    HAL_GPIO_WritePin(LCD_RST_GPIO_PORT, LCD_RST_GPIO_PIN, GPIO_PIN_RESET);
}

/**
 * @brief 通过 PB0 TIM3_CH3 打开 LCD 背光。
 *
 * TIM3_CH3 在 Core/Src/tim.c 中配置的 Period 为 300。将 CCR 设置为 ARR
 * 可以得到满占空比，然后 HAL_TIM_PWM_Start 会在 PB0 输出 PWM 波形。 */
static void LCD_BLK_ON(void)
{
    __HAL_TIM_SET_COMPARE(&LCD_BLK_TIM, LCD_BLK_TIM_CHANNEL, __HAL_TIM_GET_AUTORELOAD(&LCD_BLK_TIM));
    (void)HAL_TIM_PWM_Start(&LCD_BLK_TIM, LCD_BLK_TIM_CHANNEL);
}

/**
 * @brief 通过 SPI1 向 LCD 写入 1 个字节。
 *
 * 这个小封装由 LCD_IO_WriteCommand 使用：当 DC 设置为命令模式后，
 * 只需要发送 1 个命令字节。
 *
 * @param data 要发送的字节
 */
static void LCD_IO_WriteDataByte(uint8_t data)
{
    (void)HAL_SPI_Transmit(&LCD_SPI, &data, 1, LCD_SPI_TIMEOUT_MS);
}

/**
 * @brief 初始化 LCD IO 状态、打开背光并执行硬件复位时序。
 *
 * 本函数假设 main.c 中已经执行过 MX_GPIO_Init、MX_SPI1_Init 和 MX_TIM3_Init。
 * 它不会重新配置引脚，只会驱动已有的 GPIO/PWM 输出完成 LCD 启动时序。 */
static void LCD_IO_Init(void)
{
    LCD_BLK_ON();

    CS_IDLE();
    LCD_IO_Delay(20);  // 复位脉冲延时
    CS_ACTIVE();       // 将 CS 引脚置低
    LCD_IO_Delay(20);  // 复位脉冲延时

    //* RESX 硬件复位引脚时序
    RESX_IDLE();      // 将 RESX 引脚置高
    LCD_IO_Delay(50); // 复位脉冲延时
    RESX_ACTIVE();    // 将 RESX 引脚置低
    LCD_IO_Delay(50); // 复位脉冲延时
    RESX_IDLE();      // 将 RESX 引脚置高
    LCD_IO_Delay(50); // 复位脉冲延时
}

/**
 * @brief 通过 SPI1 向 LCD 写入数据字节。
 *
 * ST7789 命令通常遵循下面的流程：
 * 1. LCD_IO_WriteCommand(command)
 * 2. LCD_IO_WriteData(parameter_buffer, parameter_length)
 *
 * 发送数据前，PB9/LCD_DC 会被置高。PB8/LCD_CS 只在 SPI 传输期间被拉低。
 *
 * @param data 数据缓冲区指针
 * @param length 要发送的字节数 */
static void LCD_IO_WriteData(uint8_t *data, uint8_t length)
{
    if ((data == NULL) || (length == 0U)) {
    CS_IDLE();
        return;
    }

    DC_DATA();
    CS_ACTIVE();
    (void)HAL_SPI_Transmit(&LCD_SPI, data, length, LCD_SPI_TIMEOUT_MS);
    CS_IDLE();
}

/**
 * @brief 通过 SPI1 阻塞方式向 LCD 写入一段数据缓冲区。
 *
 * 这个函数用于较大的连续数据发送，例如矩形填充时的一块颜色缓冲区。
 *
 * @param data 数据缓冲区指针
 * @param length 要发送的字节数 */
#if (ST7789_FILL_MODE == ST7789_FILL_MODE_BUFFER)
static void LCD_IO_WriteDatas(uint8_t *data, uint16_t length)
{
    if ((data == NULL) || (length == 0U)) {
    CS_IDLE();
        return;
    }

    DC_DATA();
    CS_ACTIVE();
    (void)HAL_SPI_Transmit(&LCD_SPI, data, length, LCD_SPI_TIMEOUT_MS);
    CS_IDLE();
}
#endif

/**
 * @brief 通过 SPI1 DMA 非阻塞方式向 LCD 写入一段数据缓冲区。
 *
 * 本函数只启动 DMA 传输，不等待传输完成。DMA 完成后由 HAL_SPI_TxCpltCallback
 * 释放 LCD 片选，并通过 st7789_TxCpltCallback 通知上层。
 *
 * @param data 数据缓冲区指针
 * @param length 要发送的字节数 */
#if (ST7789_FILL_MODE == ST7789_FILL_MODE_DMA)
static void LCD_IO_WriteDatasDMA(uint8_t *data, uint16_t length)
{
    if ((data == NULL) || (length == 0U)) {
    CS_IDLE();
        st7789_TxCpltCallback();
        return;
    }

    if (HAL_SPI_GetState(&LCD_SPI) != HAL_SPI_STATE_READY) {
    CS_IDLE();
        st7789_TxCpltCallback();
        return;
    }

    DC_DATA();
    CS_ACTIVE();
    lcd_dma_notify_enabled = 1U;

    if (HAL_SPI_Transmit_DMA(&LCD_SPI, data, length) != HAL_OK) {
        lcd_dma_notify_enabled = 0U;
    CS_IDLE();
        st7789_TxCpltCallback();
    }
}
#endif

/**
 * @brief 通过 SPI1 DMA 阻塞方式向 LCD 写入一段数据缓冲区。
 *
 * 这个函数保留给初始化清屏、纯色填充等 BSP 内部流程使用，避免这些流程在循环分块发送时
 * 因 DMA 尚未完成而覆盖下一次传输。
 *
 * @param data 数据缓冲区指针
 * @param length 要发送的字节数 */
#if (ST7789_FILL_MODE == ST7789_FILL_MODE_DMA)
static void LCD_IO_WriteDatasDMA_Blocking(uint8_t *data, uint16_t length)
{
    uint32_t tickStart;

    if ((data == NULL) || (length == 0U)) {
    CS_IDLE();
        return;
    }

    DC_DATA();
    CS_ACTIVE();

    if (HAL_SPI_Transmit_DMA(&LCD_SPI, data, length) != HAL_OK) {
    CS_IDLE();
        return;
    }

    tickStart = HAL_GetTick();
    while (HAL_SPI_GetState(&LCD_SPI) != HAL_SPI_STATE_READY) {
        if ((HAL_GetTick() - tickStart) > LCD_SPI_TIMEOUT_MS) {
            (void)HAL_SPI_Abort(&LCD_SPI);
            break;
        }
    }

    CS_IDLE();
}
#endif

/**
 * @brief SPI DMA 发送完成后的弱回调。
 *
 * 显示移植层可以重写这个函数，用来接收 LCD 像素 DMA 传输完成事件。 */
__weak void st7789_TxCpltCallback(void)
{
}

/**
 * @brief HAL SPI 发送完成回调。
 *
 * 该函数在 SPI/DMA 中断上下文中执行，只释放片选并通知上层事件，不直接调用 LVGL API。
 *
 * @param hspi SPI 句柄 */
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance == LCD_SPI.Instance) {
    CS_IDLE();
        if (lcd_dma_notify_enabled != 0U) {
            lcd_dma_notify_enabled = 0U;
            st7789_TxCpltCallback();
        }
    }
}

/**
 * @brief HAL SPI 错误回调。
 *
 * DMA 传输异常时释放片选并通知上层，避免 LVGL 一直等待刷新完成。
 *
 * @param hspi SPI 句柄 */
void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance == LCD_SPI.Instance) {
    CS_IDLE();
        if (lcd_dma_notify_enabled != 0U) {
            lcd_dma_notify_enabled = 0U;
            st7789_TxCpltCallback();
        }
    }
}

/**
 * @brief 通过 SPI1 向 LCD 写入 1 个命令字节。
 *
 * 发送命令前，PB9/LCD_DC 会被置低。PB8/LCD_CS 只在该命令字节传输期间被拉低。
 *
 * @param command ST7789 命令 */
static void LCD_IO_WriteCommand(uint8_t command)
{
    DC_COMMAND();
    CS_ACTIVE();
    LCD_IO_WriteDataByte(command);
    CS_IDLE();
}

/**
 * @brief LCD 操作毫秒级延时。
 *
 * ST7789 复位和初始化命令之间有些步骤需要毫秒级等待。
 * HAL_Delay 在 HAL_Init 完成后使用 STM32 HAL tick 实现延时。
 *
 * @param delay 延时时间，单位 ms
 */
static void LCD_IO_Delay(uint32_t delay)
{
    HAL_Delay(delay);
}
