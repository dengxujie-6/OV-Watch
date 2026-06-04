#ifndef BSP_KEY_H
#define BSP_KEY_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BSP_KEY_BACK = 0,
    BSP_KEY_SCREEN,
} BSP_KeyId_t;

/**
 * @brief 初始化板载按键 GPIO。
 *
 * BSP Key 模块负责具体 GPIO 端口、引脚、上下拉和有效电平配置。
 */
void BSP_Key_Init(void);

/**
 * @brief 读取指定板载按键是否按下。
 *
 * @param key 按键逻辑编号。
 * @return 1 表示按下，0 表示未按下或编号无效。
 */
uint8_t BSP_Key_IsPressed(BSP_KeyId_t key);

#ifdef __cplusplus
}
#endif

#endif /* BSP_KEY_H */
