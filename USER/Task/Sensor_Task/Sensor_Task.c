#include "Sensor_Task.h"

#include "cmsis_os2.h"
#include "bsp_mpu6050.h"
#include "hwaccess.h"
#include "low_power.h"

#define SENSOR_TASK_LOW_RATE_PERIOD_MS            500U
#define SENSOR_TASK_DRDY_TIMEOUT_MS               200U
#define SENSOR_TASK_ENV_REFRESH_PERIOD_MS         1000U
#define SENSOR_TASK_STATIC_TO_SLEEP_MS            15000U
#define SENSOR_TASK_MOTION_ACCEL_DELTA_MG         120
#define SENSOR_TASK_MOTION_GYRO_X10_DPS           120
#define SENSOR_TASK_FLAG_MPU_INT                  0x0001U

typedef enum {
    SENSOR_TASK_MODE_LOW_RATE = 0,
    SENSOR_TASK_MODE_DATA_READY,
} Sensor_TaskMode_t;

static Sensor_TaskMode_t sensor_task_mode = SENSOR_TASK_MODE_LOW_RATE;
static uint32_t sensor_task_last_env_refresh_ms;
static uint32_t sensor_task_last_motion_ms;
static uint8_t sensor_task_motion_baseline_valid;
static HwAccess_Vector3i16_t sensor_task_last_accel_mg;
static HwAccess_Vector3i16_t sensor_task_last_gyro_x10_dps;

static void Sensor_Task_RefreshEnvironment(void);
static void Sensor_Task_RefreshMpuSample(void);
static void Sensor_Task_SwitchToDataReadyMode(void);
static void Sensor_Task_SwitchToLowRateMode(void);
static uint8_t Sensor_Task_IsMotionDetected(void);
static int16_t Sensor_Task_AbsI16(int16_t value);

extern osThreadId_t sensorTaskHandle;

/**
 * @brief 周期刷新传感器缓存数据。
 *
 * 该任务运行在普通 FreeRTOS 任务上下文，可以调用阻塞式 HwAccess 采样接口；
 * UI/LVGL 任务只读取 HwAccess 缓存，避免页面刷新时直接访问 ADC。
 */
void Sensor_Task(void *argument)
{
    uint32_t wake_flags;

    (void)argument;

    Sensor_Task_RefreshEnvironment();
    Sensor_Task_RefreshMpuSample();
    sensor_task_last_motion_ms = osKernelGetTickCount();

    for(;;) {
        wake_flags = LowPower_ConsumeWakeFlags();
        if(wake_flags != 0UL) {
            Sensor_Task_SwitchToDataReadyMode();
            Sensor_Task_RefreshEnvironment();
            Sensor_Task_RefreshMpuSample();
            sensor_task_last_motion_ms = osKernelGetTickCount();
        }

        if(sensor_task_mode == SENSOR_TASK_MODE_DATA_READY) {
            uint32_t flags = osThreadFlagsWait(SENSOR_TASK_FLAG_MPU_INT,
                                               osFlagsWaitAny,
                                               SENSOR_TASK_DRDY_TIMEOUT_MS);
            if((flags & SENSOR_TASK_FLAG_MPU_INT) != 0U) {
                Sensor_Task_RefreshMpuSample();
                if(Sensor_Task_IsMotionDetected() != 0U) {
                    sensor_task_last_motion_ms = osKernelGetTickCount();
                }
            }

            if((osKernelGetTickCount() - sensor_task_last_env_refresh_ms) >= SENSOR_TASK_ENV_REFRESH_PERIOD_MS) {
                Sensor_Task_RefreshEnvironment();
            }

            if((osKernelGetTickCount() - sensor_task_last_motion_ms) >= SENSOR_TASK_STATIC_TO_SLEEP_MS) {
                Sensor_Task_SwitchToLowRateMode();
                LowPower_RequestSleep();
            }
        } else {
            Sensor_Task_RefreshEnvironment();
            Sensor_Task_RefreshMpuSample();

            if(Sensor_Task_IsMotionDetected() != 0U) {
                sensor_task_last_motion_ms = osKernelGetTickCount();
                Sensor_Task_SwitchToDataReadyMode();
            } else if((osKernelGetTickCount() - sensor_task_last_motion_ms) >= SENSOR_TASK_STATIC_TO_SLEEP_MS) {
                LowPower_RequestSleep();
            }

            osDelay(SENSOR_TASK_LOW_RATE_PERIOD_MS);
        }
    }
}

/**
 * @brief 在 EXTI 回调里通知 Sensor_Task 处理一次 MPU6050 INT。
 */
void Sensor_Task_NotifyMpuInterruptFromISR(void)
{
    if(sensorTaskHandle != NULL) {
        (void)osThreadFlagsSet(sensorTaskHandle, SENSOR_TASK_FLAG_MPU_INT);
    }
}

/**
 * @brief 刷新低频环境传感器缓存。
 */
static void Sensor_Task_RefreshEnvironment(void)
{
    if(HwAccess.power.update_battery_cache != 0) {
        HwAccess.power.update_battery_cache();
    }

    if(HwAccess.aht21.update_cache != 0) {
        (void)HwAccess.aht21.update_cache();
    }

    if(HwAccess.lsm303dlhc.update_cache != 0) {
        (void)HwAccess.lsm303dlhc.update_cache();
    }

    sensor_task_last_env_refresh_ms = osKernelGetTickCount();
}

/**
 * @brief 刷新 MPU6050 缓存，并把加速度/角速度保存为静止判定输入。
 */
static void Sensor_Task_RefreshMpuSample(void)
{
    HwAccess_Vector3i16_t accel_mg;
    HwAccess_Vector3i16_t gyro_x10_dps;

    if(HwAccess.mpu6050.update_cache != 0) {
        (void)HwAccess.mpu6050.update_cache();
    }

    if((HwAccess.mpu6050.get_accel_mg != NULL) &&
       (HwAccess.mpu6050.get_gyro_x10_dps != NULL) &&
       (HwAccess.mpu6050.get_accel_mg(&accel_mg) == 0) &&
       (HwAccess.mpu6050.get_gyro_x10_dps(&gyro_x10_dps) == 0)) {
        sensor_task_last_accel_mg = accel_mg;
        sensor_task_last_gyro_x10_dps = gyro_x10_dps;
        sensor_task_motion_baseline_valid = 1U;
    }
}

/**
 * @brief 切换到 MPU6050 Data Ready 采样模式。
 */
static void Sensor_Task_SwitchToDataReadyMode(void)
{
    if(sensor_task_mode == SENSOR_TASK_MODE_DATA_READY) {
        return;
    }

    (void)BSP_MPU6050_DisableWakeOnMotion();
    (void)BSP_MPU6050_EnableDataReadyInterrupt();
    sensor_task_mode = SENSOR_TASK_MODE_DATA_READY;
    sensor_task_motion_baseline_valid = 0U;
}

/**
 * @brief 切换到低频静止监测模式。
 */
static void Sensor_Task_SwitchToLowRateMode(void)
{
    if(sensor_task_mode == SENSOR_TASK_MODE_LOW_RATE) {
        return;
    }

    (void)BSP_MPU6050_DisableDataReadyInterrupt();
    (void)BSP_MPU6050_EnableWakeOnMotion();
    sensor_task_mode = SENSOR_TASK_MODE_LOW_RATE;
    sensor_task_motion_baseline_valid = 0U;
}

/**
 * @brief 基于最近两次加速度和当前角速度判断是否存在明显运动。
 */
static uint8_t Sensor_Task_IsMotionDetected(void)
{
    HwAccess_Vector3i16_t accel_mg;
    HwAccess_Vector3i16_t gyro_x10_dps;
    int16_t accel_dx;
    int16_t accel_dy;
    int16_t accel_dz;

    if((HwAccess.mpu6050.get_accel_mg == NULL) ||
       (HwAccess.mpu6050.get_gyro_x10_dps == NULL) ||
       (HwAccess.mpu6050.get_accel_mg(&accel_mg) != 0) ||
       (HwAccess.mpu6050.get_gyro_x10_dps(&gyro_x10_dps) != 0)) {
        return 0U;
    }

    if(sensor_task_motion_baseline_valid == 0U) {
        sensor_task_last_accel_mg = accel_mg;
        sensor_task_last_gyro_x10_dps = gyro_x10_dps;
        sensor_task_motion_baseline_valid = 1U;
        return 0U;
    }

    accel_dx = Sensor_Task_AbsI16((int16_t)(accel_mg.x - sensor_task_last_accel_mg.x));
    accel_dy = Sensor_Task_AbsI16((int16_t)(accel_mg.y - sensor_task_last_accel_mg.y));
    accel_dz = Sensor_Task_AbsI16((int16_t)(accel_mg.z - sensor_task_last_accel_mg.z));

    sensor_task_last_accel_mg = accel_mg;
    sensor_task_last_gyro_x10_dps = gyro_x10_dps;

    if((accel_dx >= SENSOR_TASK_MOTION_ACCEL_DELTA_MG) ||
       (accel_dy >= SENSOR_TASK_MOTION_ACCEL_DELTA_MG) ||
       (accel_dz >= SENSOR_TASK_MOTION_ACCEL_DELTA_MG) ||
       (Sensor_Task_AbsI16(gyro_x10_dps.x) >= SENSOR_TASK_MOTION_GYRO_X10_DPS) ||
       (Sensor_Task_AbsI16(gyro_x10_dps.y) >= SENSOR_TASK_MOTION_GYRO_X10_DPS) ||
       (Sensor_Task_AbsI16(gyro_x10_dps.z) >= SENSOR_TASK_MOTION_GYRO_X10_DPS)) {
        return 1U;
    }

    return 0U;
}

/**
 * @brief 取 int16_t 绝对值。
 */
static int16_t Sensor_Task_AbsI16(int16_t value)
{
    return (value >= 0) ? value : (int16_t)(-value);
}
