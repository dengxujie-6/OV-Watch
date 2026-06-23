#ifndef EM7028_REG_H
#define EM7028_REG_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* EM7028 7 位 I2C 从机地址 */
#define EM7028_I2C_ADDR_7BIT                 0x24U

/* EM7028 产品 ID 期望值 */
#define EM7028_PRODUCT_ID_VALUE              0x36U

/* PID 只读寄存器地址 */
#define EM7028_REG_PID                       0x00U

/* 模式配置寄存器地址 */
#define EM7028_REG_CONFIGURE                 0x01U

/* 中断配置寄存器地址 */
#define EM7028_REG_INTERRUPT                 0x02U

/* HRS2 偏移寄存器地址 */
#define EM7028_REG_HRS2_DATA_OFFSET          0x08U

/* HRS2 增益控制寄存器地址 */
#define EM7028_REG_HRS2_GAIN_CTRL            0x0AU

/* HRS1 连续模式控制寄存器地址 */
#define EM7028_REG_HRS1_CTRL                 0x0DU

/* 中断控制寄存器地址 */
#define EM7028_REG_INT_CTRL                  0x0EU

/* HRS1 原始 PPG 低字节寄存器地址 */
#define EM7028_REG_HRS1_DATA0_L              0x28U

/* HRS1 原始 PPG 高字节寄存器地址 */
#define EM7028_REG_HRS1_DATA0_H              0x29U

/* CONFIGURE 寄存器中 HRS2 使能位 */
#define EM7028_CONFIGURE_HRS2_EN_MASK        (1U << 7)

/* CONFIGURE 寄存器中 HRS1 使能位 */
#define EM7028_CONFIGURE_HRS1_EN_MASK        (1U << 3)

/* HRS1 增益选择：1 倍 */
#define EM7028_HRS1_GAIN_X1                  (0U << 7)

/* HRS1 增益选择：5 倍 */
#define EM7028_HRS1_GAIN_X5                  (1U << 7)

/* HRS1 量程选择：1 倍 */
#define EM7028_HRS1_RANGE_X1                 (0U << 6)

/* HRS1 量程选择：8 倍 */
#define EM7028_HRS1_RANGE_X8                 (1U << 6)

/* HRS1 采样周期：1.5625 ms */
#define EM7028_HRS1_FREQ_1P5625MS            (0U << 3)

/* HRS1 采样周期：3.125 ms */
#define EM7028_HRS1_FREQ_3P125MS             (1U << 3)

/* HRS1 采样周期：6.25 ms */
#define EM7028_HRS1_FREQ_6P25MS              (2U << 3)

/* HRS1 采样周期：12.5 ms */
#define EM7028_HRS1_FREQ_12P5MS              (3U << 3)

/* HRS1 采样周期：25 ms */
#define EM7028_HRS1_FREQ_25MS                (4U << 3)

/* HRS1 采样周期：50 ms */
#define EM7028_HRS1_FREQ_50MS                (5U << 3)

/* HRS1 采样周期：100 ms */
#define EM7028_HRS1_FREQ_100MS               (6U << 3)

/* HRS1 采样周期：200 ms */
#define EM7028_HRS1_FREQ_200MS               (7U << 3)

/* HRS1 ADC 分辨率：10 位 */
#define EM7028_HRS1_RES_10BIT                (0U << 1)

/* HRS1 ADC 分辨率：12 位 */
#define EM7028_HRS1_RES_12BIT                (1U << 1)

/* HRS1 ADC 分辨率：14 位 */
#define EM7028_HRS1_RES_14BIT                (2U << 1)

/* HRS1 ADC 分辨率：16 位 */
#define EM7028_HRS1_RES_16BIT                (3U << 1)

/* bit0=0，选择 IR mode */
#define EM7028_HRS1_MODE_IR                  (0U << 0)

/* bit0=1，选择 HRS1 mode */
#define EM7028_HRS1_MODE_ENABLE              (1U << 0)

/* 默认 HRS1 控制值：量程 8 倍、1.5625ms、16 位、HRS1 mode，对应 0x47 */
#define EM7028_HRS1_DEFAULT_CTRL_VALUE       (EM7028_HRS1_GAIN_X1 | \
                                              EM7028_HRS1_RANGE_X8 | \
                                              EM7028_HRS1_FREQ_1P5625MS | \
                                              EM7028_HRS1_RES_16BIT | \
                                              EM7028_HRS1_MODE_ENABLE)

/* HRS2 偏移默认值：0 偏移 */
#define EM7028_HRS2_DATA_OFFSET_DEFAULT      0x00U

/* HRS2 增益控制默认值：0x7F */
#define EM7028_HRS2_GAIN_CTRL_DEFAULT        0x7FU

/* 中断控制默认值：关闭中断，LED 编程电流 2.5mA */
#define EM7028_INT_CTRL_DEFAULT              0x00U

/* HRS_CFG 关闭 HRS1/HRS2 的默认值 */
#define EM7028_CONFIGURE_DISABLE_ALL         0x00U

/* HRS_CFG 仅开启 HRS1 的参考值 */
#define EM7028_CONFIGURE_ENABLE_HRS1         0x08U

#ifdef __cplusplus
}
#endif

#endif /* EM7028_REG_H */
