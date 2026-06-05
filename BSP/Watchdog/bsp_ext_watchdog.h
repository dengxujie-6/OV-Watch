#ifndef BSP_EXT_WATCHDOG_H
#define BSP_EXT_WATCHDOG_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化外部硬件看门狗 GPIO。
 *
 * PB1 为 Dog_EN，低电平打开看门狗，高电平关闭看门狗；PB2 为 WDI，
 * 通过周期翻转电平完成喂狗。本函数只配置 GPIO，并默认保持看门狗关闭，
 * 由上层在喂狗任务启动后显式使能。
 */
void BSP_ExtWatchdog_Init(void);

/**
 * @brief 打开外部硬件看门狗。
 */
void BSP_ExtWatchdog_Enable(void);

/**
 * @brief 关闭外部硬件看门狗。
 */
void BSP_ExtWatchdog_Disable(void);

/**
 * @brief 翻转 WDI 引脚完成一次喂狗边沿。
 */
void BSP_ExtWatchdog_Feed(void);

#ifdef __cplusplus
}
#endif

#endif /* BSP_EXT_WATCHDOG_H */
