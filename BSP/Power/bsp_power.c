#include "bsp_power.h"

#include "main.h"

#define BSP_POWER_EN_GPIO_PORT              GPIOA
#define BSP_POWER_EN_GPIO_PIN               GPIO_PIN_3

#define BSP_POWER_CHARG_GPIO_PORT           GPIOA
#define BSP_POWER_CHARG_GPIO_PIN            GPIO_PIN_2
#define BSP_POWER_CHARG_ACTIVE_LEVEL        GPIO_PIN_SET

#define BSP_POWER_BAT_ADC                   ADC1
#define BSP_POWER_BAT_ADC_CHANNEL           ADC_CHANNEL_1
#define BSP_POWER_BAT_ADC_TIMEOUT_MS        10U
#define BSP_POWER_BAT_ADC_FULL_SCALE        4095U
#define BSP_POWER_BAT_ADC_VREF_MV           3300U

#ifndef BSP_POWER_BATTERY_DIVIDER_NUMERATOR
#define BSP_POWER_BATTERY_DIVIDER_NUMERATOR 2U
#endif

#ifndef BSP_POWER_BATTERY_DIVIDER_DENOMINATOR
#define BSP_POWER_BATTERY_DIVIDER_DENOMINATOR 1U
#endif

#if (BSP_POWER_BATTERY_DIVIDER_DENOMINATOR == 0U)
#error "BSP_POWER_BATTERY_DIVIDER_DENOMINATOR must not be 0"
#endif

static ADC_HandleTypeDef power_adc_handle;
static uint8_t power_adc_initialized;

volatile uint32_t g_bsp_power_debug_open_count;
volatile uint32_t g_bsp_power_debug_close_count;
volatile uint32_t g_bsp_power_debug_last_action;
volatile uint32_t g_bsp_power_debug_pa3_level;
volatile uint32_t g_bsp_power_debug_pa3_odr;

static void BSP_Power_GPIO_Init(void);
static void BSP_Power_ADC_Init(void);
static void BSP_Power_UpdateDebugState(uint32_t action_tag);

/**
 * @brief 初始化 Power 模块 GPIO，并拉高 PA3 维持系统电源。
 */
void BSP_Power_Open(void)
{
    BSP_Power_GPIO_Init();
    BSP_Power_ADC_Init();

    // PA3 是电源保持脚，初始化完成后立即输出高电平，避免系统掉电。
    HAL_GPIO_WritePin(BSP_POWER_EN_GPIO_PORT, BSP_POWER_EN_GPIO_PIN, GPIO_PIN_SET);
    g_bsp_power_debug_open_count++;
    BSP_Power_UpdateDebugState(1U);
}

void BSP_Power_Close(void)
{
    BSP_Power_GPIO_Init();
    HAL_GPIO_WritePin(BSP_POWER_EN_GPIO_PORT, BSP_POWER_EN_GPIO_PIN, GPIO_PIN_RESET);
    g_bsp_power_debug_close_count++;
    BSP_Power_UpdateDebugState(2U);
}

/**
 * @brief 读取 PA2 CHARG 充电检测状态。
 *
 * @return 1 表示高电平有效，0 表示低电平或模块尚未初始化。
 */
uint8_t BSP_Power_IsCharging(void)
{
    BSP_Power_GPIO_Init();

    return (HAL_GPIO_ReadPin(BSP_POWER_CHARG_GPIO_PORT,
                             BSP_POWER_CHARG_GPIO_PIN) == BSP_POWER_CHARG_ACTIVE_LEVEL) ? 1U : 0U;
}

/**
 * @brief 通过 ADC1_IN1 读取 PA1 电池电压检测值。
 *
 * @return 按配置分压比换算后的电压，单位 mV；采样失败时返回 0。
 */
uint16_t BSP_Power_ReadBatteryVoltageMv(void)
{
    uint32_t adc_raw;
    uint32_t voltage_mv;

    BSP_Power_ADC_Init();

    if(power_adc_initialized == 0U) {
        return 0U;
    }

    if(HAL_ADC_Start(&power_adc_handle) != HAL_OK) {
        return 0U;
    }

    if(HAL_ADC_PollForConversion(&power_adc_handle, BSP_POWER_BAT_ADC_TIMEOUT_MS) != HAL_OK) {
        (void)HAL_ADC_Stop(&power_adc_handle);
        return 0U;
    }

    adc_raw = HAL_ADC_GetValue(&power_adc_handle);
    (void)HAL_ADC_Stop(&power_adc_handle);

    // 先换算 PA1 ADC 引脚电压，再按外部分压系数还原电池端电压。
    voltage_mv = (adc_raw * BSP_POWER_BAT_ADC_VREF_MV) / BSP_POWER_BAT_ADC_FULL_SCALE;
    voltage_mv = (voltage_mv * BSP_POWER_BATTERY_DIVIDER_NUMERATOR) /
                 BSP_POWER_BATTERY_DIVIDER_DENOMINATOR;

    if(voltage_mv > 65535U) {
        return 65535U;
    }

    return (uint16_t)voltage_mv;
}

/**
 * @brief 初始化 PA3 POWER_EN、PA2 CHARG 和 PA1 ADC 模拟输入。
 */
static void BSP_Power_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();

    HAL_GPIO_WritePin(BSP_POWER_EN_GPIO_PORT, BSP_POWER_EN_GPIO_PIN, GPIO_PIN_SET);

    GPIO_InitStruct.Pin = BSP_POWER_EN_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(BSP_POWER_EN_GPIO_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = BSP_POWER_CHARG_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(BSP_POWER_CHARG_GPIO_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    BSP_Power_UpdateDebugState(3U);
}

/**
 * @brief 初始化 ADC1 单通道轮询采样，用于 PA1 电池电压检测。
 */
static void BSP_Power_ADC_Init(void)
{
    ADC_ChannelConfTypeDef channel_config = {0};

    if(power_adc_initialized != 0U) {
        return;
    }

    __HAL_RCC_ADC1_CLK_ENABLE();

    power_adc_handle.Instance = BSP_POWER_BAT_ADC;
    power_adc_handle.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
    power_adc_handle.Init.Resolution = ADC_RESOLUTION_12B;
    power_adc_handle.Init.ScanConvMode = DISABLE;
    power_adc_handle.Init.ContinuousConvMode = DISABLE;
    power_adc_handle.Init.DiscontinuousConvMode = DISABLE;
    power_adc_handle.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    power_adc_handle.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    power_adc_handle.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    power_adc_handle.Init.NbrOfConversion = 1U;
    power_adc_handle.Init.DMAContinuousRequests = DISABLE;
    power_adc_handle.Init.EOCSelection = ADC_EOC_SINGLE_CONV;

    if(HAL_ADC_Init(&power_adc_handle) != HAL_OK) {
        return;
    }

    channel_config.Channel = BSP_POWER_BAT_ADC_CHANNEL;
    channel_config.Rank = 1U;
    channel_config.SamplingTime = ADC_SAMPLETIME_144CYCLES;

    if(HAL_ADC_ConfigChannel(&power_adc_handle, &channel_config) != HAL_OK) {
        return;
    }

    power_adc_initialized = 1U;
}

/**
 * @brief 记录 PA3 保持脚当前软件可见状态，便于在调试器中观察。
 *
 * @param action_tag 最近一次动作标记：1=Open，2=Close，3=GPIO_Init。
 */
static void BSP_Power_UpdateDebugState(uint32_t action_tag)
{
    g_bsp_power_debug_last_action = action_tag;
    g_bsp_power_debug_pa3_level =
        (uint32_t)HAL_GPIO_ReadPin(BSP_POWER_EN_GPIO_PORT, BSP_POWER_EN_GPIO_PIN);
    g_bsp_power_debug_pa3_odr =
        ((BSP_POWER_EN_GPIO_PORT->ODR & BSP_POWER_EN_GPIO_PIN) != 0U) ? 1U : 0U;
}
