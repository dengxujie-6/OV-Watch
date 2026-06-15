/**
 * @file CST816T.c
 * @brief CST816T 触摸芯片驱动实现。
 */

#include "CST816T.h"

#include <string.h>

#include "I2CVirtual.h"
#include "stm32f4xx_hal.h"

#define CST816T_I2C_ADDR_7BIT       0x15U
#define CST816T_WRITE_ADDR          ((CST816T_I2C_ADDR_7BIT << 1) | 0U)
#define CST816T_READ_ADDR           ((CST816T_I2C_ADDR_7BIT << 1) | 1U)

#define CST816T_REG_GESTURE_ID      0x01U
#define CST816T_REG_CHIP_ID         0xA7U
#define CST816T_REG_MOTION_MASK     0xECU
#define CST816T_REG_AUTO_SLEEP_TIME 0xF9U
#define CST816T_REG_IRQ_CTL         0xFAU
#define CST816T_REG_DIS_AUTOSLEEP   0xFEU

#define CST816T_TP_SDA_PORT         'B'
#define CST816T_TP_SDA_PIN          4U
#define CST816T_TP_SCL_PORT         'B'
#define CST816T_TP_SCL_PIN          6U

#define CST816T_TP_RST_PORT         GPIOA
#define CST816T_TP_RST_PIN          GPIO_PIN_15

volatile int CST816T_DebugLastError;
volatile uint8_t CST816T_DebugChipID;
volatile uint8_t CST816T_DebugInfo[4];
volatile uint8_t CST816T_DebugConfig[7];
volatile uint8_t CST816T_DebugRaw[6];
volatile uint8_t CST816T_DebugRegs[16];
volatile uint16_t CST816T_DebugX;
volatile uint16_t CST816T_DebugY;
volatile uint8_t CST816T_DebugPressed;

static int CST816T_ReadRegs(uint8_t reg, uint8_t * data, uint8_t len);
static int CST816T_WriteReg(uint8_t reg, uint8_t value);
static void CST816T_Reset(void);
static void CST816T_SelectBus(void);

/**
 * @brief 初始化 CST816T 触摸芯片。
 *
 * @return 0 表示成功，负数表示通信失败。
 */
int CST816T_Init(void)
{
    uint8_t chip_id = 0U;

    I2C_Virtual_ConfigPort(CST816T_TP_SDA_PORT,
                           CST816T_TP_SDA_PIN,
                           CST816T_TP_SCL_PORT,
                           CST816T_TP_SCL_PIN);
    I2C_Virtual_SwitchBus(CST816T_TP_SDA_PORT,
                          CST816T_TP_SDA_PIN,
                          CST816T_TP_SCL_PORT,
                          CST816T_TP_SCL_PIN);
    I2C_Virtual_Init();

    CST816T_Reset();

    if(CST816T_ReadChipID(&chip_id) == 0) {
        CST816T_DebugChipID = chip_id;
    }
    (void)CST816T_ReadRegs(0xA7U, (uint8_t *)CST816T_DebugInfo, sizeof(CST816T_DebugInfo));

    (void)CST816T_WriteReg(CST816T_REG_DIS_AUTOSLEEP, 0x01U);
    (void)CST816T_WriteReg(CST816T_REG_AUTO_SLEEP_TIME, 0xFFU);
    (void)CST816T_WriteReg(CST816T_REG_MOTION_MASK, 0x01U);
    (void)CST816T_WriteReg(CST816T_REG_IRQ_CTL, 0x41U);
    (void)CST816T_ReadRegs(CST816T_REG_AUTO_SLEEP_TIME,
                           (uint8_t *)CST816T_DebugConfig,
                           sizeof(CST816T_DebugConfig));

    return 0;
}

/**
 * @brief 读取 CST816T 芯片 ID。
 *
 * @param chip_id 用于保存芯片 ID 的指针，不能为 NULL。
 * @return 0 表示成功，负数表示失败。
 */
int CST816T_ReadChipID(uint8_t * chip_id)
{
    if(chip_id == NULL) {
        return -1;
    }

    return CST816T_ReadRegs(CST816T_REG_CHIP_ID, chip_id, 1U);
}

/**
 * @brief 重新唤醒触摸芯片。
 *
 * @return 0 表示完成唤醒流程。
 */
int CST816T_Wakeup(void)
{
    CST816T_SelectBus();
    CST816T_Reset();
    I2C_Virtual_Init();
    (void)CST816T_WriteReg(CST816T_REG_DIS_AUTOSLEEP, 0x01U);
    (void)CST816T_WriteReg(CST816T_REG_AUTO_SLEEP_TIME, 0xFFU);
    (void)CST816T_WriteReg(CST816T_REG_MOTION_MASK, 0x01U);
    (void)CST816T_WriteReg(CST816T_REG_IRQ_CTL, 0x41U);
    return 0;
}

/**
 * @brief 读取当前触摸点。
 *
 * CST816T 的坐标数据从 0x01 开始连续读取：
 * 0x01 为手势，0x02 为触摸点数量，0x03~0x06 为 X/Y 坐标。
 *
 * @param point 用于保存触摸点状态的指针，不能为 NULL。
 * @return 0 表示成功，负数表示失败。
 */
int CST816T_ReadTouch(CST816T_TouchPoint_t * point)
{
    uint8_t buf[6];
    uint16_t x;
    uint16_t y;
    uint8_t finger_num;

    if(point == NULL) {
        CST816T_DebugLastError = -1;
        return -1;
    }

    memset(point, 0, sizeof(*point));

    CST816T_DebugLastError = CST816T_ReadRegs(CST816T_REG_GESTURE_ID, buf, sizeof(buf));
    if(CST816T_DebugLastError != 0) {
        CST816T_DebugPressed = 0U;
        return -2;
    }

    memcpy((void *)CST816T_DebugRaw, buf, sizeof(buf));

    finger_num = (uint8_t)(buf[1] & 0x0FU);
    if(finger_num == 0U) {
        point->is_pressed = 0U;
        CST816T_DebugPressed = 0U;
        return 0;
    }

    x = (uint16_t)(((uint16_t)(buf[2] & 0x0FU) << 8) | buf[3]);
    y = (uint16_t)(((uint16_t)(buf[4] & 0x0FU) << 8) | buf[5]);

    if(x >= CST816T_LCD_WIDTH) {
        x = CST816T_LCD_WIDTH - 1U;
    }
    if(y >= CST816T_LCD_HEIGHT) {
        y = CST816T_LCD_HEIGHT - 1U;
    }

    point->is_pressed = 1U;
    point->x = x;
    point->y = y;
    point->gesture = (CST816T_Gesture_t)buf[0];
    CST816T_DebugPressed = 1U;
    CST816T_DebugX = x;
    CST816T_DebugY = y;

    return 0;
}

static int CST816T_ReadRegs(uint8_t reg, uint8_t * data, uint8_t len)
{
    uint8_t i;

    if((data == NULL) || (len == 0U)) {
        return -1;
    }

    CST816T_SelectBus();

    I2C_Virtual_Start();
    if(I2C_Virtual_SendByte(CST816T_WRITE_ADDR) == 0U) {
        I2C_Virtual_Stop();
        return -2;
    }
    if(I2C_Virtual_SendByte(reg) == 0U) {
        I2C_Virtual_Stop();
        return -3;
    }

    I2C_Virtual_Start();
    if(I2C_Virtual_SendByte(CST816T_READ_ADDR) == 0U) {
        I2C_Virtual_Stop();
        return -4;
    }

    for(i = 0U; i < len; i++) {
        data[i] = I2C_Virtual_RcvByte();
        if(i + 1U < len) {
            I2C_Virtual_Ack();
        }
        else {
            I2C_Virtual_NoAck();
        }
    }

    I2C_Virtual_Stop();
    return 0;
}

static int CST816T_WriteReg(uint8_t reg, uint8_t value)
{
    CST816T_SelectBus();

    I2C_Virtual_Start();
    if(I2C_Virtual_SendByte(CST816T_WRITE_ADDR) == 0U) {
        I2C_Virtual_Stop();
        return -1;
    }
    if(I2C_Virtual_SendByte(reg) == 0U) {
        I2C_Virtual_Stop();
        return -2;
    }
    if(I2C_Virtual_SendByte(value) == 0U) {
        I2C_Virtual_Stop();
        return -3;
    }

    I2C_Virtual_Stop();
    return 0;
}

static void CST816T_Reset(void)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitStructure.Pin = CST816T_TP_RST_PIN;
    GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStructure.Pull = GPIO_NOPULL;
    GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(CST816T_TP_RST_PORT, &GPIO_InitStructure);

    HAL_GPIO_WritePin(CST816T_TP_RST_PORT, CST816T_TP_RST_PIN, GPIO_PIN_RESET);
    HAL_Delay(10U);
    HAL_GPIO_WritePin(CST816T_TP_RST_PORT, CST816T_TP_RST_PIN, GPIO_PIN_SET);
    HAL_Delay(120U);
}

static void CST816T_SelectBus(void)
{
    I2C_Virtual_SwitchBus(CST816T_TP_SDA_PORT,
                          CST816T_TP_SDA_PIN,
                          CST816T_TP_SCL_PORT,
                          CST816T_TP_SCL_PIN);
}
