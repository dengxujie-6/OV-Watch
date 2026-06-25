#ifndef LOW_POWER_H
#define LOW_POWER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LOW_POWER_WAKE_SOURCE_KEY2   (1UL << 0)
#define LOW_POWER_WAKE_SOURCE_MPU    (1UL << 1)

/**
 * @brief 在 EXTI 回调中记录低功耗唤醒源。
 *
 * @param gpio_pin 进入中断的 GPIO 引脚号。
 */
void LowPower_HandleWakeupIrq(uint16_t gpio_pin);

/**
 * @brief 收拢非唤醒外设后进入 MCU Sleep，并在唤醒后恢复系统。
 *
 * @return 本次唤醒源位图，组合 LOW_POWER_WAKE_SOURCE_xxx。
 */
uint32_t LowPower_EnterSleep(void);

/**
 * @brief 读取并清除最近一次 Sleep 返回后的唤醒源位图。
 *
 * @return 组合 LOW_POWER_WAKE_SOURCE_xxx；0 表示没有新的唤醒结果。
 */
uint32_t LowPower_ConsumeWakeFlags(void);
uint8_t LowPower_TakeWakeRefreshRequest(void);

#ifdef __cplusplus
}
#endif

#endif /* LOW_POWER_H */
