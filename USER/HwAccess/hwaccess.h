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
 * @brief 电池状态访问接口。
 *
 * 该接口只返回已经缓存或抽象后的电池状态，不在 UI 刷新路径里直接采样 ADC。
 */
typedef struct Batterystruct_typedef
{
    uint8_t (*get_percent)(void);  /**< 获取电池电量百分比，范围 0~100。 */
} obj_Battery;

/**
 * @brief 顶层硬件访问对象。
 *
 * 后续新增硬件模块时，应以模块操作表的形式加入这里，任务层和 UI 层统一通过该对象访问硬件。
 */
typedef struct hwaccess_typedef
{
    obj_Lcd lcd;  /**< LCD 操作表。 */
    obj_Key key;  /**< 按键操作表。 */
    obj_Battery battery;  /**< 电池状态访问接口。 */
} obj_HwAccess;

/**
 * @brief 全局硬件访问入口。
 */
extern obj_HwAccess HwAccess;

#ifdef __cplusplus
}
#endif

#endif
