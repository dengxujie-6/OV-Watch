#include "HardWare_Init_Task.h"

#include "cmsis_os2.h"
#include "hwaccess.h"

static osThreadId_t hardWareInitTaskHandle;

static const osThreadAttr_t hardWareInitTaskAttributes = {
    .name = "hwInitTask",
    .stack_size = 512 * 4,
    .priority = (osPriority_t)osPriorityAboveNormal,
};

static void HardWare_Init_Task(void *argument);

/**
 * @brief 创建硬件初始化任务。
 */
void HardWare_Init_Task_Create(void)
{
    hardWareInitTaskHandle = osThreadNew(HardWare_Init_Task, NULL, &hardWareInitTaskAttributes);
}

/**
 * @brief 初始化板级硬件模块。
 *
 * 这个任务只访问硬件接口，不创建 GUI 任务，也不调用 LVGL API。
 */
static void HardWare_Init_Task(void *argument)
{
    (void)argument;

    // 通过全局硬件接口访问 LCD，不直接调用 BSP 驱动。
    HwAccess.lcd.init();

    osThreadExit();
}
