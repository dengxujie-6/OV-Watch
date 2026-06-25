#ifndef BSP_POWER_H
#define BSP_POWER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化并拉高电源保持引脚。
 *
 * PA3 POWER_EN 为高电平保持电源。本函数会初始化 PA3/PA2/PA1 相关硬件，
 * 并把 PA3 置为高电平。
 */
void BSP_Power_Open(void);
void BSP_Power_Close(void);

/**
 * @brief 读取充电检测引脚状态。
 *
 * @return 1 表示 PA2 CHARG 为高电平，正在充电；0 表示未检测到充电。
 */
uint8_t BSP_Power_IsCharging(void);

/**
 * @brief 读取电池电压检测值。
 *
 * @return 电池电压检测值，单位 mV；ADC 失败时返回 0。
 */
uint16_t BSP_Power_ReadBatteryVoltageMv(void);

#ifdef __cplusplus
}
#endif

#endif /* BSP_POWER_H */
