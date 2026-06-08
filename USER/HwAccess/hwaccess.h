#ifndef HWACCESS_H
#define HWACCESS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LCD_WIDTH  240U
#define LCD_HEIGHT 280U

typedef enum {
    HWACCESS_KEY_BACK = 0,
    HWACCESS_KEY_SCREEN,
} HwAccess_KeyId_t;

/**
 * @brief LCD 硬件操作表。
 *
 * LCD 模块向任务层暴露初始化、显示开关和背光控制；刷屏窗口和像素传输仍由 LVGL 移植层使用。
 */
typedef struct Lcdstruct_typedef
{
    void (*init)(void);  /**< 初始化 LCD 和触摸硬件。 */
    void (*deinit)(void);  /**< 反初始化 LCD。 */
    void (*set_backlight)(uint8_t brightness);  /**< 设置 LCD 背光亮度，范围 0~100。 */
    void (*display_on)(void);  /**< 打开 LCD 显示输出。 */
    void (*display_off)(void);  /**< 关闭 LCD 显示输出。 */
} obj_Lcd;

/**
 * @brief 按键硬件操作表。
 *
 * 任务层只关心“哪个按键是否按下”，不直接持有 GPIO 端口、引脚和有效电平。
 */
typedef struct Keystruct_typedef
{
    void (*init)(void);  /**< 初始化按键输入硬件。 */
    uint8_t (*is_pressed)(HwAccess_KeyId_t key);  /**< 读取指定按键当前是否按下。 */
} obj_Key;

/**
 * @brief 电源与充电状态访问接口。
 *
 * 任务层通过该接口保持 POWER_EN、读取 CHARG 和电池电压，不直接访问 GPIO/ADC。
 */
typedef struct Powerstruct_typedef
{
    void (*open)(void);  /**< 打开并保持系统电源。 */
    uint8_t (*is_charging)(void);  /**< 读取充电检测状态，1 表示高电平有效。 */
    uint16_t (*get_battery_voltage_mv)(void);  /**< 读取电池电压检测值，单位 mV。 */
} obj_Power;

/**
 * @brief 外部硬件看门狗操作表。
 *
 * PB1 Dog_EN 低电平打开、高电平关闭；PB2 WDI 通过周期翻转完成喂狗。
 */
typedef struct Watchdogstruct_typedef
{
    void (*init)(void);  /**< 初始化外部看门狗 GPIO，默认保持关闭。 */
    void (*enable)(void);  /**< 打开外部硬件看门狗。 */
    void (*disable)(void);  /**< 关闭外部硬件看门狗。 */
    void (*feed)(void);  /**< 翻转 WDI 完成一次喂狗。 */
} obj_Watchdog;

/**
 * @brief 外部 PROM 读写操作表。
 *
 * PROM 底层使用 PA11/PA12 软件 I2C 和 BL24C02F EEPROM 驱动，上层只通过字节地址访问。
 */
typedef struct Promstruct_typedef
{
    void (*init)(void);  /**< 初始化 PROM 专用 I2C 总线。 */
    int (*probe)(void);  /**< 探测 PROM 是否响应 ACK。 */
    int (*read)(uint8_t addr, uint8_t * data, uint16_t len);  /**< 从 PROM 连续读取字节。 */
    int (*write)(uint8_t addr, const uint8_t * data, uint16_t len);  /**< 向 PROM 连续写入字节。 */
    int (*read_byte)(uint8_t addr, uint8_t * value);  /**< 读取 PROM 单字节。 */
    int (*write_byte)(uint8_t addr, uint8_t value);  /**< 写入 PROM 单字节。 */
} obj_Prom;

/**
 * @brief 顶层硬件访问对象。
 *
 * 后续新增硬件模块时，应以模块操作表的形式加入这里，任务层和 UI 层统一通过该对象访问硬件。
 */
typedef struct hwaccess_typedef
{
    obj_Lcd lcd;  /**< LCD 操作表。 */
    obj_Key key;  /**< 按键操作表。 */
    obj_Power power;  /**< 电源与充电状态访问接口。 */
    obj_Watchdog watchdog;  /**< 外部硬件看门狗操作表。 */
    obj_Prom prom;  /**< 外部 PROM 读写操作表。 */
} obj_HwAccess;

/**
 * @brief 全局硬件访问入口。
 */
extern obj_HwAccess HwAccess;

#ifdef __cplusplus
}
#endif

#endif
