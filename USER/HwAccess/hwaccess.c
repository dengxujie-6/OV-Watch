#include "hwaccess.h"

#include "bsp_ext_watchdog.h"
#include "bsp_bluetooth.h"
#include "bsp_key.h"
#include "bsp_power.h"
#include "bsp_prom.h"
#include "CST816T.h"
#include "st7789v.h"

#define HWACCESS_BATTERY_MIN_MV 2750U
#define HWACCESS_BATTERY_MAX_MV 4200U

static void HwAccess_Lcd_Init(void);
static uint8_t HwAccess_Key_IsPressed(HwAccess_KeyId_t key);
static void HwAccess_Power_UpdateBatteryCache(void);
static uint16_t HwAccess_Power_GetBatteryVoltageMv(void);
static uint8_t HwAccess_Power_GetBatteryPercent(void);
static uint8_t HwAccess_Power_IsBatteryValid(void);

static volatile uint16_t hwaccess_battery_voltage_mv;
static volatile uint8_t hwaccess_battery_percent;
static volatile uint8_t hwaccess_battery_valid;

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
 * @brief 采样一次电池电压，并把电压和百分比写入 HwAccess 缓存。
 *
 * 该函数由普通任务调用，内部通过 BSP Power 完成 PA1 ADC 读取；UI 页面只读取缓存，
 * 不在 LVGL 刷新路径里触发 ADC 轮询。
 */
static void HwAccess_Power_UpdateBatteryCache(void)
{
    uint16_t voltage_mv = BSP_Power_ReadBatteryVoltageMv();
    uint8_t percent;

    if(voltage_mv == 0U) {
        return;
    }

    if(voltage_mv <= HWACCESS_BATTERY_MIN_MV) {
        percent = 0U;
    } else if(voltage_mv >= HWACCESS_BATTERY_MAX_MV) {
        percent = 100U;
    } else {
        percent = (uint8_t)(((uint32_t)(voltage_mv - HWACCESS_BATTERY_MIN_MV) * 100U) /
                            (HWACCESS_BATTERY_MAX_MV - HWACCESS_BATTERY_MIN_MV));
    }

    hwaccess_battery_voltage_mv = voltage_mv;
    hwaccess_battery_percent = percent;
    hwaccess_battery_valid = 1U;
}

/**
 * @brief 从缓存读取最近一次有效电池电压。
 * @return 电池端电压，单位 mV；尚未采样成功时返回 0。
 */
static uint16_t HwAccess_Power_GetBatteryVoltageMv(void)
{
    return (hwaccess_battery_valid != 0U) ? hwaccess_battery_voltage_mv : 0U;
}

/**
 * @brief 从缓存读取最近一次电池电量百分比。
 * @return 电量百分比，范围 0~100；尚未采样成功时返回 0。
 */
static uint8_t HwAccess_Power_GetBatteryPercent(void)
{
    return (hwaccess_battery_valid != 0U) ? hwaccess_battery_percent : 0U;
}

/**
 * @brief 查询电池缓存是否已有有效采样。
 * @return 1 表示缓存有效，0 表示尚未成功采样。
 */
static uint8_t HwAccess_Power_IsBatteryValid(void)
{
    return hwaccess_battery_valid;
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
    .power = {
        .open = BSP_Power_Open,
        .is_charging = BSP_Power_IsCharging,
        .update_battery_cache = HwAccess_Power_UpdateBatteryCache,
        .get_battery_voltage_mv = HwAccess_Power_GetBatteryVoltageMv,
        .get_battery_percent = HwAccess_Power_GetBatteryPercent,
        .is_battery_valid = HwAccess_Power_IsBatteryValid,
    },
    .watchdog = {
        .init = BSP_ExtWatchdog_Init,
        .enable = BSP_ExtWatchdog_Enable,
        .disable = BSP_ExtWatchdog_Disable,
        .feed = BSP_ExtWatchdog_Feed,
    },
    .prom = {
        .init = BSP_PROM_Init,
        .probe = BSP_PROM_Probe,
        .read = BSP_PROM_Read,
        .write = BSP_PROM_Write,
        .read_byte = BSP_PROM_ReadByte,
        .write_byte = BSP_PROM_WriteByte,
    },
    .bluetooth = {
        .init = BSP_BlueTooth_Init,
        .enable = BSP_BlueTooth_Enable,
        .disable = BSP_BlueTooth_Disable,
        .is_enabled = BSP_BlueTooth_IsEnabled,
        .send = BSP_BlueTooth_Send,
        .send_dma = BSP_BlueTooth_SendDma,
        .is_tx_busy = BSP_BlueTooth_IsTxBusy,
        .take_tx_done = BSP_BlueTooth_TakeTxDone,
        .receive = BSP_BlueTooth_Receive,
    },
};
