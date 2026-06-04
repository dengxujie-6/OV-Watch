/**
 * @file board_hw.h
 * @brief UI 页面使用的板级硬件访问接口。
 */

#ifndef BOARD_HW_H
#define BOARD_HW_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t (*GetPercent)(void);  /**< 获取已缓存的电池电量百分比。 */
} BoardHW_Battery_t;

typedef struct {
    BoardHW_Battery_t battery;    /**< 电池状态访问接口。 */
} BoardHW_t;

extern BoardHW_t BoardHW;

#ifdef __cplusplus
}
#endif

#endif /* BOARD_HW_H */
