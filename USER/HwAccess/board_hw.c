/**
 * @file board_hw.c
 * @brief UI 页面使用的板级硬件访问接口默认实现。
 */

#include "board_hw.h"

/**
 * @brief 获取默认电池电量百分比。
 *
 * 当前工程还没有电池采样服务，这里只提供页面编译和显示用的占位数据。
 * 后续接入 ADC/电源管理任务时，应由业务层更新缓存值或替换该接口。
 *
 * @return 电池电量百分比，范围 0~100。
 */
static uint8_t BoardHW_Battery_GetPercentDefault(void)
{
    return 0U;
}

BoardHW_t BoardHW = {
    .battery = {
        .GetPercent = BoardHW_Battery_GetPercentDefault,
    },
};
