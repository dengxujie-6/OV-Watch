#ifndef HWACCESS_H
#define HWACCESS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LCD_WIDTH  240U
#define LCD_HEIGHT 280U

/**
 * @brief LCD 硬件操作表。
 *
 * LCD 模块只向应用层暴露初始化、反初始化和背光亮度控制。
 * 刷屏窗口、像素写入等底层显示传输接口不放在这里。
 */
typedef struct Lcdstruct_typedef
{
    void (*init)(void);  /**< 初始化 LCD。 */
    void (*deinit)(void);  /**< 反初始化 LCD。 */
    void (*set_backlight)(uint8_t brightness);  /**< 设置 LCD 背光亮度，范围 0~100。 */
} obj_Lcd;

/**
 * @brief 顶层硬件访问对象。
 *
 * 后续新增硬件模块时，应以模块操作表的形式加入这里。
 * 应用层通过这个总对象访问硬件模块。
 */
typedef struct hwaccess_typedef
{
    obj_Lcd lcd;  /**< LCD 操作表。 */
} obj_HwAccess;

/**
 * @brief 全局硬件访问入口。
 */
extern obj_HwAccess HwAccess;

#ifdef __cplusplus
}
#endif

#endif
