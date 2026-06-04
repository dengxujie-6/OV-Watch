#include "hwaccess.h"

#include "bsp_key.h"
#include "CST816T.h"
#include "st7789v.h"

static void HwAccess_Lcd_Init(void);
static uint8_t HwAccess_Key_IsPressed(HwAccess_KeyId_t key);
static uint8_t HwAccess_Battery_GetPercentDefault(void);

/**
 * @brief 初始化屏幕相关硬件。
 *
 * LCD 和触摸共同组成一块屏幕输入/输出设备，启动任务只需要调用这一处入口。
 */
static void HwAccess_Lcd_Init(void)
{
    st7789_Init();

    // 触摸初始化依赖虚拟 I2C 和复位脚，留在硬件访问层内统一封装。
    (void)CST816T_Init();
}

/**
 * @brief 读取指定按键是否处于按下状态。
 *
 * @param key 按键逻辑编号。
 * @return 1 表示按下，0 表示未按下或编号无效。
 */
static uint8_t HwAccess_Key_IsPressed(HwAccess_KeyId_t key)
{
    switch(key) {
        case HWACCESS_KEY_BACK:
            return BSP_Key_IsPressed(BSP_KEY_BACK);

        case HWACCESS_KEY_SCREEN:
            return BSP_Key_IsPressed(BSP_KEY_SCREEN);

        default:
            return 0U;
    }
}

/**
 * @brief 获取默认电池电量百分比。
 *
 * 当前工程还没有电池采样服务，这里只提供页面编译和显示用的占位数据。
 *
 * @return 电池电量百分比，范围 0~100。
 */
static uint8_t HwAccess_Battery_GetPercentDefault(void)
{
    return 0U;
}

/**
 * @brief 全局硬件访问对象。
 *
 * 该表把任务层可用的硬件接口绑定到具体 BSP 实现，避免任务直接依赖底层模块。
 */
obj_HwAccess HwAccess = {
    .lcd = {
        .init = HwAccess_Lcd_Init,
        .deinit = st7789_DeInit,
        .set_backlight = st7789_SetBacklight,
        .display_on = st7789_DisplayOn,
        .display_off = st7789_DisplayOff,
    },
    .key = {
        .init = BSP_Key_Init,
        .is_pressed = HwAccess_Key_IsPressed,
    },
    .battery = {
        .get_percent = HwAccess_Battery_GetPercentDefault,
    },
};
