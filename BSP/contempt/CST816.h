/**
 * @file CST816.h
 * @brief CST816 触摸芯片 BSP 接口。
 */

#ifndef __CST816_H
#define __CST816_H

#include <stdint.h>

#include "iic_hal.h"

/* CST816 引脚定义 ---------------------------------------------------------- */
#define TOUCH_RST_PIN      GPIO_PIN_15
#define TOUCH_RST_PORT     GPIOA

/* CST816 复位控制 ---------------------------------------------------------- */
#define TOUCH_RST_0        HAL_GPIO_WritePin(TOUCH_RST_PORT, TOUCH_RST_PIN, GPIO_PIN_RESET)
#define TOUCH_RST_1        HAL_GPIO_WritePin(TOUCH_RST_PORT, TOUCH_RST_PIN, GPIO_PIN_SET)

/* CST816 I2C 设备地址 ------------------------------------------------------ */
#define Device_Addr        0x15U

/* CST816 寄存器地址 -------------------------------------------------------- */
#define GestureID          0x01U
#define FingerNum          0x02U
#define XposH              0x03U
#define XposL              0x04U
#define YposH              0x05U
#define YposL              0x06U
#define ChipID             0xA7U
#define SleepMode          0xE5U
#define MotionMask         0xECU
#define IrqPluseWidth      0xEDU
#define NorScanPer         0xEEU
#define MotionSlAngle      0xEFU
#define LpAutoWakeTime     0xF4U
#define LpScanTH           0xF5U
#define LpScanWin          0xF6U
#define LpScanFreq         0xF7U
#define LpScanIdac         0xF8U
#define AutoSleepTime      0xF9U
#define IrqCtl             0xFAU
#define AutoReset          0xFBU
#define LongPressTime      0xFCU
#define IOCtl              0xFDU
#define DisAutoSleep       0xFEU

/**
 * @brief CST816 触摸坐标信息。
 */
typedef struct
{
    unsigned int X_Pos;
    unsigned int Y_Pos;
} CST816_Info;

/**
 * @brief CST816 手势 ID。
 */
typedef enum
{
    NOGESTURE   = 0x00U,
    DOWNGLIDE   = 0x01U,
    UPGLIDE     = 0x02U,
    LEFTGLIDE   = 0x03U,
    RIGHTGLIDE  = 0x04U,
    CLICK       = 0x05U,
    DOUBLECLICK = 0x0BU,
    LONGPRESS   = 0x0CU,
} GestureID_TypeDef;

/**
 * @brief 连续动作识别配置。
 */
typedef enum
{
    M_DISABLE   = 0x00U,
    EnConLR     = 0x01U,
    EnConUD     = 0x02U,
    EnDClick    = 0x03U,
    M_ALLENABLE = 0x07U,
} MotionMask_TypeDef;

/**
 * @brief CST816 中断输出控制选项。
 */
typedef enum
{
    OnceWLP  = 0x00U,
    EnMotion = 0x10U,
    EnChange = 0x20U,
    EnTouch  = 0x40U,
    EnTest   = 0x80U,
} IrqCtl_TypeDef;

extern CST816_Info CST816_Instance;
extern volatile uint8_t CST816_DebugChipID;
extern volatile uint8_t CST816_DebugFingerNum;
extern volatile uint8_t CST816_DebugXYRaw[4];
extern volatile uint16_t CST816_DebugX;
extern volatile uint16_t CST816_DebugY;

/**
 * @brief 初始化 CST816 相关 GPIO 和软件 I2C 总线。
 */
void CST816_GPIO_Init(void);

/**
 * @brief 对 CST816 执行硬件复位。
 */
void CST816_RESET(void);

/**
 * @brief 初始化 CST816 触摸芯片。
 */
void CST816_Init(void);

/**
 * @brief 读取触摸坐标，并保存到 CST816_Instance。
 */
void CST816_Get_XY_AXIS(void);

/**
 * @brief 读取 CST816 芯片 ID。
 *
 * @return 芯片 ID 寄存器的值。
 */
uint8_t CST816_Get_ChipID(void);

/**
 * @brief 读取当前触摸手指数量。
 *
 * @return FingerNum 寄存器的值。
 */
uint8_t CST816_Get_FingerNum(void);

/**
 * @brief 向 CST816 指定寄存器写入 1 字节数据。
 *
 * @param addr 寄存器地址。
 * @param dat 要写入的数据。
 */
void CST816_IIC_WriteREG(uint8_t addr, uint8_t dat);

/**
 * @brief 从 CST816 指定寄存器读取 1 字节数据。
 *
 * @param addr 寄存器地址。
 * @return 读取到的寄存器值。
 */
uint8_t CST816_IIC_ReadREG(unsigned char addr);

void CST816_Config_MotionMask(uint8_t mode);
void CST816_Config_AutoSleepTime(uint8_t time);
void CST816_Config_MotionSlAngle(uint8_t x_right_y_up_angle);
void CST816_Config_NorScanPer(uint8_t Period);
void CST816_Config_IrqPluseWidth(uint8_t Width);
void CST816_Config_LpScanTH(uint8_t TH);
void CST816_Wakeup(void);
void CST816_Sleep(void);

#endif
