#include "bsp_ext_watchdog.h"

#include "main.h"

#define BSP_EXT_WATCHDOG_EN_GPIO_PORT         GPIOB
#define BSP_EXT_WATCHDOG_EN_GPIO_PIN          GPIO_PIN_1
#define BSP_EXT_WATCHDOG_ENABLE_LEVEL         GPIO_PIN_RESET
#define BSP_EXT_WATCHDOG_DISABLE_LEVEL        GPIO_PIN_SET

#define BSP_EXT_WATCHDOG_WDI_GPIO_PORT        GPIOB
#define BSP_EXT_WATCHDOG_WDI_GPIO_PIN         GPIO_PIN_2

/**
 * @brief 初始化 PB1 Dog_EN 和 PB2 WDI。
 */
void BSP_ExtWatchdog_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOB_CLK_ENABLE();

    // 先写入安全默认电平，再切换为输出，减少初始化瞬间误打开看门狗的风险。
    HAL_GPIO_WritePin(BSP_EXT_WATCHDOG_EN_GPIO_PORT,
                      BSP_EXT_WATCHDOG_EN_GPIO_PIN,
                      BSP_EXT_WATCHDOG_DISABLE_LEVEL);
    HAL_GPIO_WritePin(BSP_EXT_WATCHDOG_WDI_GPIO_PORT,
                      BSP_EXT_WATCHDOG_WDI_GPIO_PIN,
                      GPIO_PIN_RESET);

    GPIO_InitStruct.Pin = BSP_EXT_WATCHDOG_EN_GPIO_PIN | BSP_EXT_WATCHDOG_WDI_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

/**
 * @brief 将 PB1 拉低，打开外部硬件看门狗。
 */
void BSP_ExtWatchdog_Enable(void)
{
    HAL_GPIO_WritePin(BSP_EXT_WATCHDOG_EN_GPIO_PORT,
                      BSP_EXT_WATCHDOG_EN_GPIO_PIN,
                      BSP_EXT_WATCHDOG_ENABLE_LEVEL);
}

/**
 * @brief 将 PB1 拉高，关闭外部硬件看门狗。
 */
void BSP_ExtWatchdog_Disable(void)
{
    HAL_GPIO_WritePin(BSP_EXT_WATCHDOG_EN_GPIO_PORT,
                      BSP_EXT_WATCHDOG_EN_GPIO_PIN,
                      BSP_EXT_WATCHDOG_DISABLE_LEVEL);
}

/**
 * @brief 翻转 PB2 WDI，给外部硬件看门狗提供喂狗边沿。
 */
void BSP_ExtWatchdog_Feed(void)
{
    HAL_GPIO_TogglePin(BSP_EXT_WATCHDOG_WDI_GPIO_PORT,
                       BSP_EXT_WATCHDOG_WDI_GPIO_PIN);
}
