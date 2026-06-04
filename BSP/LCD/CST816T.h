/**
 * @file CST816T.h
 * @brief CST816T 触摸芯片驱动接口。
 */

#ifndef CST816T_H
#define CST816T_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CST816T_LCD_WIDTH    240U
#define CST816T_LCD_HEIGHT   280U

typedef enum {
    CST816T_GESTURE_NONE  = 0x00,
    CST816T_GESTURE_UP    = 0x01,
    CST816T_GESTURE_DOWN  = 0x02,
    CST816T_GESTURE_LEFT  = 0x03,
    CST816T_GESTURE_RIGHT = 0x04,
    CST816T_GESTURE_CLICK = 0x05,
    CST816T_GESTURE_DOUBLE_CLICK = 0x0B,
    CST816T_GESTURE_LONG_PRESS   = 0x0C,
} CST816T_Gesture_t;

typedef struct {
    uint8_t is_pressed;          /**< 1 表示当前有触摸点。 */
    uint16_t x;                  /**< 触摸 X 坐标。 */
    uint16_t y;                  /**< 触摸 Y 坐标。 */
    CST816T_Gesture_t gesture;   /**< CST816T 上报的手势。 */
} CST816T_TouchPoint_t;

extern volatile int CST816T_DebugLastError;
extern volatile uint8_t CST816T_DebugChipID;
extern volatile uint8_t CST816T_DebugInfo[4];
extern volatile uint8_t CST816T_DebugConfig[7];
extern volatile uint8_t CST816T_DebugRaw[6];
extern volatile uint8_t CST816T_DebugRegs[16];
extern volatile uint16_t CST816T_DebugX;
extern volatile uint16_t CST816T_DebugY;
extern volatile uint8_t CST816T_DebugPressed;

/**
 * @brief 初始化 CST816T 触摸芯片。
 *
 * @return 0 表示成功，负数表示通信失败。
 */
int CST816T_Init(void);

/**
 * @brief 读取 CST816T 芯片 ID。
 *
 * @param chip_id 用于保存芯片 ID 的指针，不能为 NULL。
 * @return 0 表示成功，负数表示失败。
 */
int CST816T_ReadChipID(uint8_t * chip_id);

/**
 * @brief 重新唤醒触摸芯片。
 *
 * @return 0 表示完成唤醒流程。
 */
int CST816T_Wakeup(void);

/**
 * @brief 读取当前触摸点。
 *
 * @param point 用于保存触摸点状态的指针，不能为 NULL。
 * @return 0 表示成功，负数表示失败。
 */
int CST816T_ReadTouch(CST816T_TouchPoint_t * point);

#ifdef __cplusplus
}
#endif

#endif /* CST816T_H */
