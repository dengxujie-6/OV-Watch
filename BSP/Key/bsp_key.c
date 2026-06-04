#include "bsp_key.h"

#include "main.h"

#define BSP_KEY_BACK_GPIO_PORT      GPIOA
#define BSP_KEY_BACK_GPIO_PIN       GPIO_PIN_5
#define BSP_KEY_BACK_PRESSED_LEVEL  GPIO_PIN_RESET

#define BSP_KEY_SCREEN_GPIO_PORT      GPIOA
#define BSP_KEY_SCREEN_GPIO_PIN       GPIO_PIN_4
#define BSP_KEY_SCREEN_PRESSED_LEVEL  GPIO_PIN_SET

/**
 * @brief 初始化板载按键 GPIO。
 *
 * KEY_BACK 使用 PA5，上拉输入，按下为低电平；KEY_SCREEN 使用 PA4，下拉输入，按下为高电平。
 */
void BSP_Key_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitStruct.Pin = BSP_KEY_BACK_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(BSP_KEY_BACK_GPIO_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = BSP_KEY_SCREEN_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(BSP_KEY_SCREEN_GPIO_PORT, &GPIO_InitStruct);
}

/**
 * @brief 读取指定板载按键是否按下。
 *
 * @param key 按键逻辑编号。
 * @return 1 表示按下，0 表示未按下或编号无效。
 */
uint8_t BSP_Key_IsPressed(BSP_KeyId_t key)
{
    switch(key) {
        case BSP_KEY_BACK:
            return (HAL_GPIO_ReadPin(BSP_KEY_BACK_GPIO_PORT,
                                     BSP_KEY_BACK_GPIO_PIN) == BSP_KEY_BACK_PRESSED_LEVEL) ? 1U : 0U;

        case BSP_KEY_SCREEN:
            return (HAL_GPIO_ReadPin(BSP_KEY_SCREEN_GPIO_PORT,
                                     BSP_KEY_SCREEN_GPIO_PIN) == BSP_KEY_SCREEN_PRESSED_LEVEL) ? 1U : 0U;

        default:
            return 0U;
    }
}
