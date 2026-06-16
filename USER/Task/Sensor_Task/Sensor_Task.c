#include "Sensor_Task.h"

#include "cmsis_os2.h"
#include "hwaccess.h"

#define SENSOR_TASK_REFRESH_PERIOD_MS 500U

/**
 * @brief 周期刷新传感器缓存数据。
 *
 * 该任务运行在普通 FreeRTOS 任务上下文，可以调用阻塞式 HwAccess 采样接口；
 * UI/LVGL 任务只读取 HwAccess 缓存，避免页面刷新时直接访问 ADC。
 */
void Sensor_Task(void *argument)
{
    (void)argument;

    for(;;) {
        if(HwAccess.power.update_battery_cache != 0) {
            HwAccess.power.update_battery_cache();
        }

        if(HwAccess.aht21.update_cache != 0) {
            (void)HwAccess.aht21.update_cache();
        }

        if(HwAccess.lsm303dlhc.update_cache != 0) {
            (void)HwAccess.lsm303dlhc.update_cache();
        }

        if(HwAccess.mpu6050.update_cache != 0) {
            (void)HwAccess.mpu6050.update_cache();
        }

        osDelay(SENSOR_TASK_REFRESH_PERIOD_MS);
    }
}
