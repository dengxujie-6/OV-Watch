#include "bsp_key.h"

#include "main.h"

#define BSP_KEY_BACK_GPIO_PORT      GPIOA
#define BSP_KEY_BACK_GPIO_PIN       GPIO_PIN_5
#define BSP_KEY_BACK_PRESSED_LEVEL  GPIO_PIN_RESET

#define BSP_KEY_SCREEN_GPIO_PORT      GPIOA
#define BSP_KEY_SCREEN_GPIO_PIN       GPIO_PIN_4
#define BSP_KEY_SCREEN_PRESSED_LEVEL  GPIO_PIN_SET


/**
 * @brief 鍒濆鍖栨澘杞芥寜閿?GPIO銆? *
 * KEY_BACK 浣跨敤 PA5锛屼笂鎷夎緭鍏ワ紝鎸変笅涓轰綆鐢靛钩锛汯EY_SCREEN 浣跨敤 PA4锛屼笅鎷夎緭鍏ワ紝鎸変笅涓洪珮鐢靛钩锛? */
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
 * @brief 璇诲彇鎸囧畾鏉胯浇鎸夐敭鏄惁鎸変笅銆? *
 * @param key 鎸夐敭閫昏緫缂栧彿銆? * @return 1 琛ㄧず鎸変笅锛? 琛ㄧず鏈寜涓嬫垨缂栧彿鏃犳晥銆? */
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
