/**
 * @file st7789v.c
 * @author Renato Freitas (freitas-renato@outlook.com)
 * @brief ST7789V LCD 椹卞姩鍑芥暟瀹炵幇
 * @version 0.1
 * @date 2021-03-24
 * 
 * @note 鏈枃浠舵部鐢?STMicroelectronics Nucleo/Discovery 鏉垮崱涓殑 LCD_IO 鎺ュ彛椋庢牸
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
/*                      杩欓儴鍒咷PIO锛孲PI锛孴IM澶栬鐢盋ubMX閰嶇疆濂斤紝鎴栬€呭湪鍏朵粬鍦版柟鎵嬪姩鍒濆鍖?             */
/*************************************************************************************************/
/* LCD 鎺у埗寮曡剼銆傝繖浜?GPIO 寮曡剼鍦?Core/Src/gpio.c 涓畬鎴愬垵濮嬪寲銆?*/
#define LCD_DC_GPIO_PORT        GPIOB
#define LCD_DC_GPIO_PIN         GPIO_PIN_9     // PB9锛氬懡浠?鏁版嵁閫夋嫨 LCD_DC
#define LCD_CS_GPIO_PORT        GPIOB
#define LCD_CS_GPIO_PIN         GPIO_PIN_8     // PB8锛氱墖閫?LCD_CS
#define LCD_RST_GPIO_PORT       GPIOB
#define LCD_RST_GPIO_PIN        GPIO_PIN_7     // PB7锛氱‖浠跺浣?LCD_RST

/* PB0 琚厤缃负 TIM3_CH3 PWM 杈撳嚭锛岀敤浜?LCD 鑳屽厜銆?*/
#define LCD_BLK_TIM             htim3
#define LCD_BLK_TIM_CHANNEL     TIM_CHANNEL_3

/* PB3/PB5 鍦?Core/Src/spi.c 涓厤缃负 SPI1_SCK/SPI1_MOSI銆?*/
#define LCD_SPI                 hspi1
#define LCD_SPI_TIMEOUT_MS      500U

#if ((ST7789_FILL_MODE != ST7789_FILL_MODE_PIXEL) && \
     (ST7789_FILL_MODE != ST7789_FILL_MODE_BUFFER) && \
     (ST7789_FILL_MODE != ST7789_FILL_MODE_DMA))
#error "Invalid ST7789_FILL_MODE"
#endif

static volatile uint8_t lcd_dma_notify_enabled;

/*************************************************************************************************/
/*                      杩欓儴鍒咷PIO锛孲PI锛孴IM澶栬鐢盋ubMX閰嶇疆濂斤紝鎴栬€呭湪鍏朵粬鍦版柟鎵嬪姩鍒濆鍖?             */
/*************************************************************************************************/


/**
 * @brief ST7789 鍛戒护缁撴瀯浣撱€? */
typedef struct {
    uint8_t command;  // 鍛戒护 ID
    uint16_t waitMs;  // 鍛戒护鍙戦€佸悗鐨勫欢鏃舵椂闂达紝鍗曚綅 ms
    uint8_t dataSize; // 鍛戒护鍙傛暟鏁版嵁闀垮害
    uint8_t *data;
} st7789_command_t;

/* ST7789 鍛戒护/鏁版嵁鍐欏叆娴佺▼浣跨敤鐨勬枃浠跺唴绉佹湁鍑芥暟銆?*/
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
 * @brief ST7789 鍒濆鍖栨祦绋嬶紝閰嶇疆鏄剧ず鎺ュ彛鍜屾樉绀哄弬鏁般€? * 
 */
void st7789_Init(void) {
    LCD_IO_Init();

    //* CASET 璁剧疆鏄剧ず瀹藉害鑼冨洿
    const uint8_t caset[4] = {
        0x00, 0x00,
        (ST7789_LCD_WIDTH - 1) >> 8, (ST7789_LCD_WIDTH - 1) & 0xFF
    };

    //* RASET 璁剧疆鏄剧ず楂樺害鑼冨洿
    const uint8_t raset[4] = {
        0x00, 0x00,
        (ST7789_LCD_HEIGHT - 1) >> 8, (ST7789_LCD_HEIGHT - 1) & 0xFF
    };

    const st7789_command_t initSequence[] = {
        // 杩涘叆鐫＄湢
        {ST7789_CMD_SLPIN, 10, 0, NULL},                    // 杩涘叆鐫＄湢
        {ST7789_CMD_SWRESET, 200, 0, NULL},                 // 杞欢澶嶄綅
        {ST7789_CMD_SLPOUT, 120, 0, NULL},                  // 閫€鍑虹潯鐪?
        {ST7789_CMD_CMD2EN, 100, 0, NULL},

        {ST7789_CMD_MADCTL, 0, 1, ( uint8_t *)"\x00"},      // 椤?鍒楀湴鍧€椤哄簭
        {ST7789_CMD_COLMOD, 0, 1, ( uint8_t *)"\x55"},      // 16 浣?RGB 妯″紡

        // //* 娣诲姞 VSYNC銆丠SYNC 閰嶇疆
        // {ST7789_CMD_RGBCTRL, 0, 3, (uint8_t *)"\x42\x08\x3c"},

        {ST7789_CMD_INVON, 0, 0, NULL},                     // 鎵撳紑鏄剧ず鍙嶈壊
        {ST7789_CMD_CASET, 0, 4, ( uint8_t *)caset},        // 璁剧疆瀹藉害鑼冨洿
        {ST7789_CMD_RASET, 0, 4, ( uint8_t *)raset},        // 璁剧疆楂樺害鑼冨洿

        // Porch 鍙傛暟璁剧疆
        {ST7789_CMD_PORCTRL, 0, 5, ( uint8_t *)"\x0c\x0c\x00\x33\x33"},
        // 璁剧疆 VGH 涓?12.54V锛孷GL 涓?-9.6V
        {ST7789_CMD_GCTRL, 0, 1, ( uint8_t *)"\x35"},
        // 璁剧疆 VCOM 涓?1.475V
        {ST7789_CMD_VCOMS, 0, 1, ( uint8_t *)"\x1f"},
        // 浣胯兘 VDV/VRH 鎺у埗
        {ST7789_CMD_VDVVRHEN, 0, 1, ( uint8_t *)"\x01"},

        // LCM 鎺у埗
        {ST7789_CMD_LCMCTRL, 0, 1, ( uint8_t *)"\x2c"},
        // VAP(GVDD) 鐢靛帇璁＄畻锛?.45 + (VCOM + VCOM 鍋忕Щ + VDV)
        {ST7789_CMD_VRHS, 0, 1, ( uint8_t *)"\x12"},
        // 璁剧疆 VDV 涓?0V
        {ST7789_CMD_VDVSET, 0, 1, ( uint8_t *)"\x20"},
        // 璁剧疆 AVDD 涓?6.8V锛孉VCL 涓?-4.8V锛孷DDS 涓?2.3V
        {ST7789_CMD_PWCTRL1, 0, 2, ( uint8_t *)"\xa4\xa1"},
        // 璁剧疆鍒锋柊鐜囦负 60 fps
        {ST7789_CMD_FRCTR2, 0, 1, ( uint8_t *)"\x0f"},
        // 璁剧疆 Gamma 涓?2.2
        {ST7789_CMD_GAMSET, 0, 1, (uint8_t *)"\x01"},
        // Gamma 鏇茬嚎
        {ST7789_CMD_PVGAMCTRL, 0, 14, ( uint8_t *)"\xd0\x08\x11\x08\x0c\x15\x39\x33\x50\x36\x13\x14\x29\x2d"},
        {ST7789_CMD_NVGAMCTRL, 0, 14, ( uint8_t *)"\xd0\x08\x10\x08\x06\x06\x39\x44\x51\x0b\x16\x14\x2f\x31"},
        
        {ST7789_CMDLIST_END, 0, 0, NULL}                   // 鍛戒护搴忓垪缁撴潫
    };

    st7789_RunCommands(initSequence);

    LCD_IO_Delay(10);
    st7789_Clear(ST7789_BLACK);

    const st7789_command_t initSequence2[] = {
        {ST7789_CMD_RGBCTRL, 0, 3, (uint8_t *)"\x42\x08\x3c"},  // 璁剧疆 HSYNC = 0x3C锛孷SYNC = 0x80
        {ST7789_CMD_RAMCTRL, 0, 2, (uint8_t*)"\x11\xc2"},       // RAMCTRL 閫夋嫨 RGB 鎺ュ彛
        {ST7789_CMD_DISPON, 100, 0, NULL},                      // 鎵撳紑鏄剧ず
        {ST7789_CMD_SLPOUT, 100, 0, NULL},                      // 閫€鍑虹潯鐪?        
        {ST7789_CMD_RAMWR, 50, 0, NULL},                        // 寮€濮嬪啓 GRAM
        {ST7789_CMDLIST_END, 0, 0, NULL},                       // 鍛戒护搴忓垪缁撴潫
    };

    st7789_RunCommands(initSequence2);
}

void st7789_Reset(void) {
    LCD_IO_Init();
}

/**
 * @brief 鍙嶅垵濮嬪寲 LCD 鏄剧ず妯″潡銆? *
 * 鍏抽棴鏄剧ず杈撳嚭骞跺皢鑳屽厜浜害璁剧疆涓?0銆? */
void st7789_DeInit(void)
{
    st7789_DisplayOff();
    st7789_SetBacklight(0U);
}


/**
 * @brief 鎵撳紑鏄剧ず銆? * 
 */
void st7789_DisplayOn(void) {
    LCD_IO_WriteCommand(ST7789_CMD_DISPON);
}

/**
 * @brief 鍏抽棴鏄剧ず銆? * 
 */
void st7789_DisplayOff(void) {
    LCD_IO_WriteCommand(ST7789_CMD_DISPOFF);
}

/**
 * @brief 璁剧疆 LCD 鑳屽厜浜害銆? *
 * @param brightness 鑳屽厜浜害鐧惧垎姣旓紝鑼冨洿 0~100銆? */
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
 * @brief 閫氳繃 SPI 鎵ц涓€鏉?ST7789V 鍛戒护銆? * 
 * @param command 鍛戒护缁撴瀯浣撴寚閽? */
static void st7789_RunCommand(const st7789_command_t *command) {
    LCD_IO_WriteCommand(command->command);

    LCD_IO_WriteData(command->data, command->dataSize);

    if (command->waitMs > 0) {
        LCD_IO_Delay(command->waitMs);
    }
}

/**
 * @brief 鎵ц棰勫畾涔夌殑鍛戒护搴忓垪銆? * 
 * @param sequence 鍛戒护搴忓垪鏁扮粍鎸囬拡
 */
static void st7789_RunCommands(const st7789_command_t *sequence) {
    while (sequence->command != ST7789_CMDLIST_END) {
        st7789_RunCommand(sequence);
        sequence++;
    }
}

/**
 * @brief 璁剧疆鏄剧ず RAM 鍐欏儚绱犵獥鍙ｃ€? * 
 * @param xStart  姘村钩鏂瑰悜璧峰鍧愭爣
 * @param yStart  鍨傜洿鏂瑰悜璧峰鍧愭爣
 * @param xEnd    姘村钩鏂瑰悜缁撴潫鍧愭爣
 * @param yEnd    鍨傜洿鏂瑰悜缁撴潫鍧愭爣
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
 * @brief 濉厖鐭╁舰鍖哄煙銆? * 
 * @param color     16 浣?RGB565 棰滆壊鍊? * @param startX    鐭╁舰璧峰 X 鍧愭爣
 * @param startY    鐭╁舰璧峰 Y 鍧愭爣
 * @param width     鐭╁舰瀹藉害
 * @param height    鐭╁舰楂樺害
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

    //* 鏍规嵁璧风偣鍜屽楂樿缃?LCD 鍐欏叆绐楀彛
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
 * @brief 灏?LCD 鐗囬€夊紩鑴氳缃负绌洪棽鐢靛钩銆? *
 * PB8 杩炴帴鍒?LCD_CS銆傞珮鐢靛钩琛ㄧず LCD 涓嶅搷搴斿綋鍓?SPI 鎬荤嚎浼犺緭銆? */
static void CS_IDLE(void)
{
    HAL_GPIO_WritePin(LCD_CS_GPIO_PORT, LCD_CS_GPIO_PIN, GPIO_PIN_SET);
}

/**
 * @brief 灏?LCD 鐗囬€夊紩鑴氳缃负鏈夋晥鐢靛钩銆? *
 * PB8 杩炴帴鍒?LCD_CS銆備綆鐢靛钩琛ㄧず鍦?SPI 浼犺緭鍓嶉€変腑 LCD銆? */
static void CS_ACTIVE(void)
{
    HAL_GPIO_WritePin(LCD_CS_GPIO_PORT, LCD_CS_GPIO_PIN, GPIO_PIN_RESET);
}

/**
 * @brief 閫氳繃 LCD D/C 寮曡剼閫夋嫨鍛戒护浼犺緭銆? *
 * PB9 杩炴帴鍒?LCD_DC銆備綆鐢靛钩琛ㄧず鎺ヤ笅鏉ョ殑 SPI 瀛楄妭鏄懡浠ゃ€? */
static void DC_COMMAND(void)
{
    HAL_GPIO_WritePin(LCD_DC_GPIO_PORT, LCD_DC_GPIO_PIN, GPIO_PIN_RESET);
}

/**
 * @brief 閫氳繃 LCD D/C 寮曡剼閫夋嫨鏁版嵁浼犺緭銆? *
 * PB9 杩炴帴鍒?LCD_DC銆傞珮鐢靛钩琛ㄧず鎺ヤ笅鏉ョ殑 SPI 瀛楄妭鏄暟鎹€? */
static void DC_DATA(void)
{
    HAL_GPIO_WritePin(LCD_DC_GPIO_PORT, LCD_DC_GPIO_PIN, GPIO_PIN_SET);
}

/**
 * @brief 閲婃斁 LCD 纭欢澶嶄綅寮曡剼銆? *
 * PB7 杩炴帴鍒?LCD_RST銆傞珮鐢靛钩璁?ST7789 绂诲紑澶嶄綅鐘舵€併€? */
static void RESX_IDLE(void)
{
    HAL_GPIO_WritePin(LCD_RST_GPIO_PORT, LCD_RST_GPIO_PIN, GPIO_PIN_SET);
}

/**
 * @brief 鎷変綆 LCD 纭欢澶嶄綅寮曡剼銆? *
 * PB7 杩炴帴鍒?LCD_RST銆備綆鐢靛钩浼氬浣?ST7789 鎺у埗鍣ㄣ€? */
static void RESX_ACTIVE(void)
{
    HAL_GPIO_WritePin(LCD_RST_GPIO_PORT, LCD_RST_GPIO_PIN, GPIO_PIN_RESET);
}

/**
 * @brief 閫氳繃 PB0 TIM3_CH3 鎵撳紑 LCD 鑳屽厜銆? *
 * TIM3_CH3 鍦?Core/Src/tim.c 涓厤缃殑 Period 涓?300銆傚皢 CCR 璁剧疆涓?ARR
 * 鍙互寰楀埌婊″崰绌烘瘮锛岀劧鍚?HAL_TIM_PWM_Start 浼氬湪 PB0 杈撳嚭 PWM 娉㈠舰銆? */
static void LCD_BLK_ON(void)
{
    __HAL_TIM_SET_COMPARE(&LCD_BLK_TIM, LCD_BLK_TIM_CHANNEL, __HAL_TIM_GET_AUTORELOAD(&LCD_BLK_TIM));
    (void)HAL_TIM_PWM_Start(&LCD_BLK_TIM, LCD_BLK_TIM_CHANNEL);
}

/**
 * @brief 閫氳繃 SPI1 鍚?LCD 鍐欏叆 1 涓瓧鑺傘€? *
 * 杩欎釜灏忓皝瑁呯敱 LCD_IO_WriteCommand 浣跨敤锛氬綋 DC 璁剧疆涓哄懡浠ゆā寮忓悗锛? * 鍙渶瑕佸彂閫?1 涓懡浠ゅ瓧鑺傘€? *
 * @param data 瑕佸彂閫佺殑瀛楄妭
 */
static void LCD_IO_WriteDataByte(uint8_t data)
{
    (void)HAL_SPI_Transmit(&LCD_SPI, &data, 1, LCD_SPI_TIMEOUT_MS);
}

/**
 * @brief 鍒濆鍖?LCD IO 鐘舵€併€佹墦寮€鑳屽厜骞舵墽琛岀‖浠跺浣嶆椂搴忋€? *
 * 鏈嚱鏁板亣瀹?main.c 涓凡缁忔墽琛岃繃 MX_GPIO_Init銆丮X_SPI1_Init 鍜?MX_TIM3_Init銆? * 瀹冧笉浼氶噸鏂伴厤缃紩鑴氾紝鍙細椹卞姩宸叉湁鐨?GPIO/PWM 杈撳嚭瀹屾垚 LCD 鍚姩鏃跺簭銆? */
static void LCD_IO_Init(void)
{
    LCD_BLK_ON();

    CS_IDLE();
    LCD_IO_Delay(20);  // 澶嶄綅鑴夊啿寤舵椂
    CS_ACTIVE();       // 灏?CS 寮曡剼缃綆
    LCD_IO_Delay(20);  // 澶嶄綅鑴夊啿寤舵椂

    //* RESX 纭欢澶嶄綅寮曡剼鏃跺簭
    RESX_IDLE();      // 灏?RESX 寮曡剼缃珮
    LCD_IO_Delay(50); // 澶嶄綅鑴夊啿寤舵椂
    RESX_ACTIVE();    // 灏?RESX 寮曡剼缃綆
    LCD_IO_Delay(50); // 澶嶄綅鑴夊啿寤舵椂
    RESX_IDLE();      // 灏?RESX 寮曡剼缃珮
    LCD_IO_Delay(50); // 澶嶄綅鑴夊啿寤舵椂
}

/**
 * @brief 閫氳繃 SPI1 鍚?LCD 鍐欏叆鏁版嵁瀛楄妭銆? *
 * ST7789 鍛戒护閫氬父閬靛惊涓嬮潰鐨勬祦绋嬶細
 * 1. LCD_IO_WriteCommand(command)
 * 2. LCD_IO_WriteData(parameter_buffer, parameter_length)
 *
 * 鍙戦€佹暟鎹墠锛孭B9/LCD_DC 浼氳缃珮銆侾B8/LCD_CS 鍙湪 SPI 浼犺緭鏈熼棿琚媺浣庛€? *
 * @param data 鏁版嵁缂撳啿鍖烘寚閽? * @param length 瑕佸彂閫佺殑瀛楄妭鏁? */
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
 * @brief 閫氳繃 SPI1 闃诲鏂瑰紡鍚?LCD 鍐欏叆涓€娈垫暟鎹紦鍐插尯銆? *
 * 杩欎釜鍑芥暟鐢ㄤ簬杈冨ぇ鐨勮繛缁暟鎹彂閫侊紝渚嬪鐭╁舰濉厖鏃剁殑涓€鍧楅鑹茬紦鍐插尯銆? *
 * @param data 鏁版嵁缂撳啿鍖烘寚閽? * @param length 瑕佸彂閫佺殑瀛楄妭鏁? */
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
 * @brief 閫氳繃 SPI1 DMA 闈為樆濉炴柟寮忓悜 LCD 鍐欏叆涓€娈垫暟鎹紦鍐插尯銆? *
 * 鏈嚱鏁板彧鍚姩 DMA 浼犺緭锛屼笉绛夊緟浼犺緭瀹屾垚銆侱MA 瀹屾垚鍚庣敱 HAL_SPI_TxCpltCallback
 * 閲婃斁 LCD 鐗囬€夛紝骞堕€氳繃 st7789_TxCpltCallback 閫氱煡涓婂眰銆? *
 * @param data 鏁版嵁缂撳啿鍖烘寚閽堛€? * @param length 瑕佸彂閫佺殑瀛楄妭鏁般€? */
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
 * @brief 閫氳繃 SPI1 DMA 闃诲鏂瑰紡鍚?LCD 鍐欏叆涓€娈垫暟鎹紦鍐插尯銆? *
 * 杩欎釜鍑芥暟淇濈暀缁欏垵濮嬪寲娓呭睆銆佺函鑹插～鍏呯瓑 BSP 鍐呴儴娴佺▼浣跨敤锛岄伩鍏嶈繖浜涙祦绋嬪湪寰幆鍒嗗潡鍙戦€佹椂
 * 鍥?DMA 灏氭湭瀹屾垚鑰岃鐩栦笅涓€娆′紶杈撱€? *
 * @param data 鏁版嵁缂撳啿鍖烘寚閽堛€? * @param length 瑕佸彂閫佺殑瀛楄妭鏁般€? */
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
 * @brief SPI DMA 鍙戦€佸畬鎴愬悗鐨勫急鍥炶皟銆? *
 * 鏄剧ず绉绘灞傚彲浠ラ噸鍐欒繖涓嚱鏁帮紝鐢ㄦ潵鎺ユ敹 LCD 鍍忕礌 DMA 浼犺緭瀹屾垚浜嬩欢銆? */
__weak void st7789_TxCpltCallback(void)
{
}

/**
 * @brief HAL SPI 鍙戦€佸畬鎴愬洖璋冦€? *
 * 璇ュ嚱鏁板湪 SPI/DMA 涓柇涓婁笅鏂囦腑鎵ц锛屽彧閲婃斁鐗囬€夊苟閫氱煡涓婂眰浜嬩欢锛屼笉鐩存帴璋冪敤 LVGL API銆? *
 * @param hspi SPI 鍙ユ焺銆? */
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
 * @brief HAL SPI 閿欒鍥炶皟銆? *
 * DMA 浼犺緭寮傚父鏃堕噴鏀剧墖閫夊苟閫氱煡涓婂眰锛岄伩鍏?LVGL 涓€鐩寸瓑寰呭埛鏂板畬鎴愩€? *
 * @param hspi SPI 鍙ユ焺銆? */
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
 * @brief 閫氳繃 SPI1 鍚?LCD 鍐欏叆 1 涓懡浠ゅ瓧鑺傘€? *
 * 鍙戦€佸懡浠ゅ墠锛孭B9/LCD_DC 浼氳缃綆銆侾B8/LCD_CS 鍙湪璇ュ懡浠ゅ瓧鑺備紶杈撴湡闂磋鎷変綆銆? *
 * @param command ST7789 鍛戒护鐮? */
static void LCD_IO_WriteCommand(uint8_t command)
{
    DC_COMMAND();
    CS_ACTIVE();
    LCD_IO_WriteDataByte(command);
    CS_IDLE();
}

/**
 * @brief LCD 鎿嶄綔姣绾у欢鏃躲€? *
 * ST7789 澶嶄綅鍜屽垵濮嬪寲鍛戒护涔嬮棿鏈変簺姝ラ闇€瑕佹绉掔骇绛夊緟銆? * HAL_Delay 鍦?HAL_Init 瀹屾垚鍚庝娇鐢?STM32 HAL tick 瀹炵幇寤舵椂銆? *
 * @param delay 寤舵椂鏃堕棿锛屽崟浣?ms
 */
static void LCD_IO_Delay(uint32_t delay)
{
    HAL_Delay(delay);
}
