#include "Key_task.h"

#include "FreeRTOS.h"
#include "cmsis_os2.h"
#include "hwaccess.h"
#include "task.h"

#define KEY_TASK_SCAN_PERIOD_MS       5U
#define KEY_TASK_DEBOUNCE_TICKS       4U

typedef struct {
    HwAccess_KeyId_t id;
    uint8_t stable_pressed;
    uint8_t last_sample;
    uint8_t debounce_count;
} KeyScan_t;

static volatile uint32_t key_task_events;

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
 * 任务层只负责去抖和事件转换；GPIO 端口、引脚、上下拉和有效电平由 HwAccess 隔离。
 *
 * @param argument FreeRTOS 任务参数，当前未使用。
 */
void Key_Task(void *argument)
{
    KeyScan_t key1;
    KeyScan_t key2;

    (void)argument;

    HwAccess.key.init();

    key1.id = HWACCESS_KEY_BACK;
    key1.stable_pressed = HwAccess.key.is_pressed(HWACCESS_KEY_BACK);
    key1.last_sample = key1.stable_pressed;
    key1.debounce_count = 0U;

    key2.id = HWACCESS_KEY_SCREEN;
    key2.stable_pressed = HwAccess.key.is_pressed(HWACCESS_KEY_SCREEN);
    key2.last_sample = key2.stable_pressed;
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
 * @brief 扫描单个按键，并在稳定按下边沿返回 1。
 *
 * @param key 按键扫描状态结构体指针。
 * @return 1 表示检测到一次新的稳定按下，0 表示没有新按下事件。
 */
static uint8_t Key_Task_ScanPressedEdge(KeyScan_t *key)
{
    uint8_t sample = HwAccess.key.is_pressed(key->id);

    if(sample != key->last_sample) {
        key->last_sample = sample;
        key->debounce_count = 0U;
        return 0U;
    }

    if(key->debounce_count < KEY_TASK_DEBOUNCE_TICKS) {
        key->debounce_count++;
        return 0U;
    }

    if(sample == key->stable_pressed) {
        return 0U;
    }

    key->stable_pressed = sample;

    return (sample != 0U) ? 1U : 0U;
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
