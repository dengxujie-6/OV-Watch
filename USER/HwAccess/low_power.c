#include "low_power.h"

#include "cmsis_os2.h"
#include "hwaccess.h"
#include "main.h"

#include "bsp_bluetooth.h"
#include "bsp_iic.h"
#include "bsp_mpu6050.h"
#include "dma.h"
#include "gpio.h"
#include "spi.h"
#include "tim.h"

#define LOW_POWER_WAKE_KEY2_PIN          GPIO_PIN_4
#define LOW_POWER_WAKE_KEY2_PORT         GPIOA
#define LOW_POWER_WAKE_MPU_PIN           GPIO_PIN_12
#define LOW_POWER_WAKE_MPU_PORT          GPIOB
#define LOW_POWER_EXTI_IRQ_PRIORITY      5U

extern osThreadId_t keyTaskHandle;
extern osThreadId_t lvglTaskHandle;
extern osThreadId_t sensorTaskHandle;
extern osThreadId_t watchdogTaskHandle;

void SystemClock_Config(void);

static volatile uint32_t low_power_wake_flags;
static volatile uint32_t low_power_last_wake_flags;
static volatile uint8_t low_power_sleeping;
static volatile uint8_t low_power_need_wake_refresh;
static uint8_t low_power_tasks_suspended;

static void LowPower_ConfigWakeupPins(void);
static void LowPower_ClearWakeupPending(void);
static void LowPower_PreparePeripheralsForStopEntry(void);
static void LowPower_PreparePeripheralsForStopReentry(void);
static void LowPower_RestoreMinimalPeripherals(void);
static void LowPower_RestoreFullPeripherals(void);
static void LowPower_SuspendTasks(void);
static void LowPower_ResumeTasks(void);
static uint32_t LowPower_RunStopCycle(uint8_t first_entry);

/**
 * @brief 在 EXTI 回调中记录低功耗唤醒源。
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
 * @brief 收敛非唤醒外设后进入 MCU STOP，并在唤醒后恢复最小运行环境。
 *
 * @return 本次唤醒源位图，组合 LOW_POWER_WAKE_SOURCE_xxx。
 */
uint32_t LowPower_EnterStop(void)
{
    return LowPower_RunStopCycle(1U);
}

/**
 * @brief 在抬腕判定失败后再次进入 MCU STOP。
 *
 * @return 本次唤醒源位图，组合 LOW_POWER_WAKE_SOURCE_xxx。
 */
uint32_t LowPower_ReenterStop(void)
{
    return LowPower_RunStopCycle(0U);
}

/**
 * @brief 在 STOP 唤醒判定通过后恢复完整外设并继续任务调度。
 */
void LowPower_ResumeAfterStop(void)
{
    LowPower_RestoreFullPeripherals();
    LowPower_ResumeTasks();
    low_power_tasks_suspended = 0U;
}

/**
 * @brief 读取并清除最近一次 STOP 返回后的唤醒源位图。
 *
 * @return 组合 LOW_POWER_WAKE_SOURCE_xxx。
 */
uint32_t LowPower_ConsumeWakeFlags(void)
{
    uint32_t wake_flags = low_power_last_wake_flags;

    low_power_last_wake_flags = 0UL;
    return wake_flags;
}

/**
 * @brief 请求 GUI 任务在下一轮循环中执行整屏刷新。
 */
void LowPower_RequestWakeRefresh(void)
{
    low_power_need_wake_refresh = 1U;
}

uint8_t LowPower_TakeWakeRefreshRequest(void)
{
    uint8_t need_refresh = low_power_need_wake_refresh;

    low_power_need_wake_refresh = 0U;
    return need_refresh;
}

/**
 * @brief 配置 PA4 和 PB12 为 STOP 唤醒 EXTI 输入。
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
 * @brief 清理进入 STOP 前遗留的 EXTI/PWR 唤醒标志。
 */
static void LowPower_ClearWakeupPending(void)
{
    __HAL_GPIO_EXTI_CLEAR_IT(LOW_POWER_WAKE_KEY2_PIN);
    __HAL_GPIO_EXTI_CLEAR_IT(LOW_POWER_WAKE_MPU_PIN);
    __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);
}

/**
 * @brief 首次进入 STOP 前关闭显示、蓝牙与非唤醒外设。
 */
static void LowPower_PreparePeripheralsForStopEntry(void)
{
    (void)BSP_MPU6050_EnableWakeOnMotion();

    if(HwAccess.watchdog.disable != NULL) {
        HwAccess.watchdog.disable();
    }

    if(HwAccess.lcd.deinit != NULL) {
        HwAccess.lcd.deinit();
    }

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
 * @brief 抬腕判定失败后再次进入 STOP 前回收最小恢复链路。
 */
static void LowPower_PreparePeripheralsForStopReentry(void)
{
    BSP_IIC_DeInit();
}

/**
 * @brief STOP 唤醒后只恢复按键、电源保持与 MPU 读取所需的最小外设。
 */
static void LowPower_RestoreMinimalPeripherals(void)
{
    if(HwAccess.power.open != NULL) {
        HwAccess.power.open();
    }

    if(HwAccess.key.init != NULL) {
        HwAccess.key.init();
    }

    BSP_IIC_Init();
}

/**
 * @brief 抬腕判定通过后恢复完整外设链路。
 */
static void LowPower_RestoreFullPeripherals(void)
{
    MX_GPIO_Init();
    MX_DMA_Init();
    MX_SPI1_Init();
    MX_TIM3_Init();

    if(HwAccess.power.open != NULL) {
        HwAccess.power.open();
    }

    if(HwAccess.key.init != NULL) {
        HwAccess.key.init();
    }

    if(HwAccess.bluetooth.init != NULL) {
        HwAccess.bluetooth.init();
    }

    if(HwAccess.lcd.init != NULL) {
        HwAccess.lcd.init();
    }

    if(HwAccess.watchdog.enable != NULL) {
        HwAccess.watchdog.enable();
    }

    LowPower_RequestWakeRefresh();
}

/**
 * @brief 挂起会访问显示和采样链路的任务。
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
 * @brief 完整唤醒后恢复被挂起的任务。
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
 * @brief 执行一次 STOP 周期，并在唤醒后恢复最小运行环境。
 *
 * @param first_entry 1 表示从正常运行态首次进入 STOP；0 表示抬腕判定失败后的再次进入。
 *
 * @return 本次唤醒源位图。
 */
static uint32_t LowPower_RunStopCycle(uint8_t first_entry)
{
    uint32_t wake_flags;

    LowPower_ConfigWakeupPins();

    if((first_entry != 0U) || (low_power_tasks_suspended == 0U)) {
        LowPower_SuspendTasks();
        low_power_tasks_suspended = 1U;
        LowPower_PreparePeripheralsForStopEntry();
    } else {
        LowPower_PreparePeripheralsForStopReentry();
    }

    low_power_wake_flags = 0UL;
    low_power_sleeping = 1U;
    LowPower_ClearWakeupPending();

    HAL_SuspendTick();
    HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);
    SystemClock_Config();
    HAL_ResumeTick();

    low_power_sleeping = 0U;
    wake_flags = low_power_wake_flags;
    low_power_wake_flags = 0UL;
    low_power_last_wake_flags = wake_flags;

    LowPower_RestoreMinimalPeripherals();

    return wake_flags;
}
