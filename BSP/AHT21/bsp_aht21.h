#ifndef BSP_AHT21_H
#define BSP_AHT21_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BSP_AHT21_I2C_ADDR_7BIT
#define BSP_AHT21_I2C_ADDR_7BIT    0x38U
#endif

/**
 * @brief AHT21 单次测量结果。
 *
 * temperature_x10_c 使用 0.1 摄氏度为单位，例如 253 表示 25.3 摄氏度；
 * humidity_x10_percent 使用 0.1%RH 为单位，例如 486 表示 48.6%RH。
 */
typedef struct {
    int16_t temperature_x10_c;
    uint16_t humidity_x10_percent;
} BSP_AHT21_Data_t;

/**
 * @brief 初始化 AHT21 所在的软件 I2C 总线并确认芯片校准状态。
 *
 * @return 0 表示初始化成功，负数表示无 ACK、初始化命令失败或状态未就绪。
 */
int BSP_AHT21_Init(void);

/**
 * @brief 探测 AHT21 默认 7 位 I2C 地址是否响应 ACK。
 *
 * @return 0 表示收到 ACK，负数表示无响应。
 */
int BSP_AHT21_Probe(void);

/**
 * @brief 触发一次 AHT21 温湿度测量并读取转换结果。
 *
 * @param data 接收测量结果的指针，不能为 NULL，调用者拥有该对象。
 * @return 0 表示成功，负数表示参数非法、总线失败、芯片忙或 CRC 校验失败。
 */
int BSP_AHT21_Read(BSP_AHT21_Data_t * data);

/**
 * @brief 向 AHT21 发送软件复位命令。
 *
 * @return 0 表示命令发送成功，负数表示总线失败。
 */
int BSP_AHT21_SoftReset(void);

#ifdef __cplusplus
}
#endif

#endif /* BSP_AHT21_H */
