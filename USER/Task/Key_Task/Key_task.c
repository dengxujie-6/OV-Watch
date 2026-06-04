#include "Key_task.h"

#include "FreeRTOS.h"
#include "cmsis_os2.h"
#include "main.h"
#include "task.h"

#define KEY_TASK_SCAN_PERIOD_MS       5U
#define KEY_TASK_DEBOUNCE_TICKS       4U

#define KEY1_GPIO_PORT                GPIOA
#define KEY1_GPIO_PIN                 GPIO_PIN_5
#define KEY1_PRESSED_LEVEL            GPIO_PIN_RESET

#define KEY2_GPIO_PORT                GPIOA
#define KEY2_GPIO_PIN                 GPIO_PIN_4
#define KEY2_PRESSED_LEVEL            GPIO_PIN_SET

typedef struct {
    GPIO_TypeDef *port;
    uint16_t pin;
    GPIO_PinState pressed_level;
    GPIO_PinState stable_level;
    GPIO_PinState last_sample;
    uint8_t debounce_count;
} KeyScan_t;

static volatile uint32_t key_task_events;

static void Key_Task_GpioInit(void);
static uint8_t Key_Task_ScanPressedEdge(KeyScan_t *key);
static void Key_Task_PostEvent(uint32_t event);

/**
 * @brief 取出按键任务产生的事件。
 *
 * 调用后会清空事件位，建议由 LVGL_Task 周期调用并在 UI 任务上下文处理。
 *
 * @return KEY_TASK_EVENT_xxx 事件位组合。
 */
uint32_t Key_Task_FetchEvents(void)
{
    uint32_t events;

    taskENTER_CRITICAL();
    events = key_task_events;
    key_task_events = 0UL;
    taskEXIT_CRITICAL();

    return events;
}

/**
 * @brief 按键扫描任务入口函数。
 *
 * KEY1 连接 PA5，按下接 GND，因此使用上拉输入并检测低电平。
 * KEY2 连接 PA4，按下接 VCC，因此使用下拉输入并检测高电平。
 *
 * @param argument FreeRTOS 任务参数，当前未使用。
 */
void Key_Task(void *argument)
{
    KeyScan_t key1;
    KeyScan_t key2;

    (void)argument;

    Key_Task_GpioInit();

    key1.port = KEY1_GPIO_PORT;
    key1.pin = KEY1_GPIO_PIN;
    key1.pressed_level = KEY1_PRESSED_LEVEL;
    key1.stable_level = HAL_GPIO_ReadPin(KEY1_GPIO_PORT, KEY1_GPIO_PIN);
    key1.last_sample = key1.stable_level;
    key1.debounce_count = 0U;

    key2.port = KEY2_GPIO_PORT;
    key2.pin = KEY2_GPIO_PIN;
    key2.pressed_level = KEY2_PRESSED_LEVEL;
    key2.stable_level = HAL_GPIO_ReadPin(KEY2_GPIO_PORT, KEY2_GPIO_PIN);
    key2.last_sample = key2.stable_level;
    key2.debounce_count = 0U;

    for(;;) {
        if(Key_Task_ScanPressedEdge(&key1) != 0U) {
            Key_Task_PostEvent(KEY_TASK_EVENT_BACK);
        }

        if(Key_Task_ScanPressedEdge(&key2) != 0U) {
            Key_Task_PostEvent(KEY_TASK_EVENT_SCREEN);
        }

        osDelay(KEY_TASK_SCAN_PERIOD_MS);
    }
}

/**
 * @brief 初始化 KEY1/KEY2 GPIO 输入模式。
 */
static void Key_Task_GpioInit(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitStruct.Pin = KEY1_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(KEY1_GPIO_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = KEY2_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(KEY2_GPIO_PORT, &GPIO_InitStruct);
}

/**
 * @brief 扫描单个按键，并在稳定按下边沿返回 1。
 *
 * @param key 按键扫描状态结构体指针。
 * @return 1 表示检测到一次新的稳定按下，0 表示没有新按下事件。
 */
static uint8_t Key_Task_ScanPressedEdge(KeyScan_t *key)
{
    GPIO_PinState sample = HAL_GPIO_ReadPin(key->port, key->pin);

    if(sample != key->last_sample) {
        key->last_sample = sample;
        key->debounce_count = 0U;
        return 0U;
    }

    if(key->debounce_count < KEY_TASK_DEBOUNCE_TICKS) {
        key->debounce_count++;
        return 0U;
    }

    if(sample == key->stable_level) {
        return 0U;
    }

    key->stable_level = sample;

    return (sample == key->pressed_level) ? 1U : 0U;
}

/**
 * @brief 投递一个按键事件。
 *
 * @param event KEY_TASK_EVENT_xxx 事件位。
 */
static void Key_Task_PostEvent(uint32_t event)
{
    taskENTER_CRITICAL();
    key_task_events |= event;
    taskEXIT_CRITICAL();
}
