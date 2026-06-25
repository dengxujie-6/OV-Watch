#include "low_power.h"

#include "cmsis_os2.h"
#include "dma.h"
#include "gpio.h"
#include "hwaccess.h"
#include "main.h"
#include "spi.h"
#include "tim.h"

#include "bsp_bluetooth.h"
#include "bsp_iic.h"
#include "bsp_mpu6050.h"

#define LOW_POWER_WAKE_KEY2_PIN          GPIO_PIN_4
#define LOW_POWER_WAKE_KEY2_PORT         GPIOA
#define LOW_POWER_WAKE_MPU_PIN           GPIO_PIN_12
#define LOW_POWER_WAKE_MPU_PORT          GPIOB
#define LOW_POWER_EXTI_IRQ_PRIORITY      5U

extern osThreadId_t keyTaskHandle;
extern osThreadId_t lvglTaskHandle;
extern osThreadId_t sensorTaskHandle;
extern osThreadId_t watchdogTaskHandle;

static volatile uint32_t low_power_wake_flags;
static volatile uint32_t low_power_last_wake_flags;
static volatile uint8_t low_power_sleeping;
static volatile uint8_t low_power_need_wake_refresh;

static void LowPower_ConfigWakeupPins(void);
static void LowPower_PreparePeripherals(void);
static void LowPower_ResumePeripherals(void);
static void LowPower_SuspendTasks(void);
static void LowPower_ResumeTasks(void);

/**
 * @brief 在 EXTI 回调中记录唤醒源。
 *
 * @param gpio_pin 进入中断的 GPIO 引脚号。
 */
void LowPower_HandleWakeupIrq(uint16_t gpio_pin)
{
    if(low_power_sleeping == 0U) {
        return;
    }

    if(gpio_pin == LOW_POWER_WAKE_KEY2_PIN) {
        low_power_wake_flags |= LOW_POWER_WAKE_SOURCE_KEY2;
    } else if(gpio_pin == LOW_POWER_WAKE_MPU_PIN) {
        low_power_wake_flags |= LOW_POWER_WAKE_SOURCE_MPU;
    }
}

/**
 * @brief 收拢非唤醒外设并进入 Sleep。
 *
 * @return 本次唤醒源位图。
 */
uint32_t LowPower_EnterSleep(void)
{
    uint32_t wake_flags;

    LowPower_ConfigWakeupPins();
    LowPower_SuspendTasks();
    LowPower_PreparePeripherals();

    low_power_wake_flags = 0UL;
    low_power_sleeping = 1U;

    __HAL_GPIO_EXTI_CLEAR_IT(LOW_POWER_WAKE_KEY2_PIN);
    __HAL_GPIO_EXTI_CLEAR_IT(LOW_POWER_WAKE_MPU_PIN);

    HAL_SuspendTick();
    HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
    HAL_ResumeTick();

    low_power_sleeping = 0U;
    wake_flags = low_power_wake_flags;
    low_power_wake_flags = 0UL;
    low_power_last_wake_flags = wake_flags;

    LowPower_ResumePeripherals();
    LowPower_ResumeTasks();

    return wake_flags;
}

/**
 * @brief 读取并清除最近一次唤醒源位图。
 *
 * @return 组合 LOW_POWER_WAKE_SOURCE_xxx。
 */
uint32_t LowPower_ConsumeWakeFlags(void)
{
    uint32_t wake_flags = low_power_last_wake_flags;

    low_power_last_wake_flags = 0UL;
    return wake_flags;
}

uint8_t LowPower_TakeWakeRefreshRequest(void)
{
    uint8_t need_refresh = low_power_need_wake_refresh;

    low_power_need_wake_refresh = 0U;
    return need_refresh;
}

/**
 * @brief 配置 PA4 和 PB12 为 Sleep 唤醒 EXTI 输入。
 */
static void LowPower_ConfigWakeupPins(void)
{
    GPIO_InitTypeDef gpio_init = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    gpio_init.Pin = LOW_POWER_WAKE_KEY2_PIN;
    gpio_init.Mode = GPIO_MODE_IT_RISING;
    gpio_init.Pull = GPIO_PULLDOWN;
    HAL_GPIO_Init(LOW_POWER_WAKE_KEY2_PORT, &gpio_init);

    gpio_init.Pin = LOW_POWER_WAKE_MPU_PIN;
    gpio_init.Mode = GPIO_MODE_IT_RISING;
    gpio_init.Pull = GPIO_PULLDOWN;
    HAL_GPIO_Init(LOW_POWER_WAKE_MPU_PORT, &gpio_init);

    HAL_NVIC_SetPriority(EXTI4_IRQn, LOW_POWER_EXTI_IRQ_PRIORITY, 0U);
    HAL_NVIC_EnableIRQ(EXTI4_IRQn);
    HAL_NVIC_SetPriority(EXTI15_10_IRQn, LOW_POWER_EXTI_IRQ_PRIORITY, 0U);
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}

/**
 * @brief 睡前暂停会访问显示和采样链路的任务。
 *
 * Sleep 现在由独立的 Power_Task 触发，因此这里需要额外挂起 LVGL 任务，
 * 避免 LCD 已经去初始化后 GUI 任务仍继续访问显示驱动。
 */
static void LowPower_SuspendTasks(void)
{
    if(lvglTaskHandle != NULL) {
        (void)osThreadSuspend(lvglTaskHandle);
    }

    if(sensorTaskHandle != NULL) {
        (void)osThreadSuspend(sensorTaskHandle);
    }

    if(keyTaskHandle != NULL) {
        (void)osThreadSuspend(keyTaskHandle);
    }

    if(watchdogTaskHandle != NULL) {
        (void)osThreadSuspend(watchdogTaskHandle);
    }
}

/**
 * @brief 唤醒后恢复任务运行。
 */
static void LowPower_ResumeTasks(void)
{
    if(lvglTaskHandle != NULL) {
        (void)osThreadResume(lvglTaskHandle);
    }

    if(keyTaskHandle != NULL) {
        (void)osThreadResume(keyTaskHandle);
    }

    if(sensorTaskHandle != NULL) {
        (void)osThreadResume(sensorTaskHandle);
    }

    if(watchdogTaskHandle != NULL) {
        (void)osThreadResume(watchdogTaskHandle);
    }
}

/**
 * @brief 睡前关闭显示、蓝牙和非唤醒 GPIO，仅保留 POWER_EN 与唤醒引脚。
 */
static void LowPower_PreparePeripherals(void)
{
    (void)BSP_MPU6050_EnableWakeOnMotion();

    HwAccess.watchdog.disable();
    HwAccess.lcd.deinit();
    BSP_BlueTooth_DeInit();

    HAL_NVIC_DisableIRQ(SPI1_IRQn);
    HAL_NVIC_DisableIRQ(DMA2_Stream2_IRQn);
    (void)HAL_SPI_DeInit(&hspi1);
    (void)HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_3);
    (void)HAL_TIM_PWM_DeInit(&htim3);
    __HAL_RCC_SPI1_CLK_DISABLE();
    __HAL_RCC_TIM3_CLK_DISABLE();
    __HAL_RCC_DMA2_CLK_DISABLE();

    BSP_IIC_DeInit();

    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_5 | GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_15);
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5 |
                           GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_13 | GPIO_PIN_14);
}

/**
 * @brief 唤醒后恢复常规 GPIO、显示、蓝牙和 MPU6050 采样配置。
 */
static void LowPower_ResumePeripherals(void)
{
    MX_GPIO_Init();
    MX_DMA_Init();
    MX_SPI1_Init();
    MX_TIM3_Init();

    HwAccess.power.open();
    HwAccess.key.init();
    HwAccess.bluetooth.init();
    HwAccess.lcd.init();
    HwAccess.lcd.set_backlight(10U);
    HwAccess.watchdog.enable();
    low_power_need_wake_refresh = 1U;
}
