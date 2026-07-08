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
#define SENSOR_TASK_RAISE_SAMPLE_PERIOD_MS        100U
#define SENSOR_TASK_RAISE_CONFIRM_COUNT           2U
#define SENSOR_TASK_RAISE_ACCEL_Y_DELTA_MG        220
#define SENSOR_TASK_RAISE_ACCEL_Z_DELTA_MG        220
#define SENSOR_TASK_RAISE_GYRO_X10_DPS            150

typedef enum {
    SENSOR_TASK_MODE_LOW_RATE = 0,
    SENSOR_TASK_MODE_DATA_READY,
} Sensor_TaskMode_t;

static Sensor_TaskMode_t sensor_task_mode = SENSOR_TASK_MODE_LOW_RATE;
static uint32_t sensor_task_last_env_refresh_ms;
static uint32_t sensor_task_last_motion_ms;
static uint8_t sensor_task_motion_baseline_valid;
static HwAccess_Vector3i16_t sensor_task_current_accel_mg;
static HwAccess_Vector3i16_t sensor_task_current_gyro_x10_dps;
static HwAccess_Vector3i16_t sensor_task_last_accel_mg;
static HwAccess_Vector3i16_t sensor_task_last_gyro_x10_dps;

static void Sensor_Task_RefreshEnvironment(void);
static uint8_t Sensor_Task_RefreshMpuSample(void);
static void Sensor_Task_SwitchToDataReadyMode(void);
static void Sensor_Task_SwitchToLowRateMode(void);
static uint8_t Sensor_Task_IsMotionDetected(void);
static int16_t Sensor_Task_AbsI16(int16_t value);
static uint8_t Sensor_Task_IsRaiseSampleMatched(const HwAccess_Vector3i16_t * base_accel,
                                                const HwAccess_Vector3i16_t * accel_mg,
                                                const HwAccess_Vector3i16_t * gyro_x10_dps);

extern osThreadId_t sensorTaskHandle;

/**
 * @brief 周期刷新传感器缓存数据。
 *
 * @param argument FreeRTOS 任务参数，当前未使用。
 */
void Sensor_Task(void *argument)
{
    uint32_t wake_flags;

    (void)argument;

    Sensor_Task_RefreshEnvironment();
    (void)Sensor_Task_RefreshMpuSample();
    sensor_task_last_motion_ms = osKernelGetTickCount();

    for(;;) {
        wake_flags = LowPower_ConsumeWakeFlags();
        if(wake_flags != 0UL) {
            Sensor_Task_SwitchToDataReadyMode();
            Sensor_Task_RefreshEnvironment();
            (void)Sensor_Task_RefreshMpuSample();
            sensor_task_last_motion_ms = osKernelGetTickCount();
        }

        if(sensor_task_mode == SENSOR_TASK_MODE_DATA_READY) {
            uint32_t flags = osThreadFlagsWait(SENSOR_TASK_FLAG_MPU_INT,
                                               osFlagsWaitAny,
                                               SENSOR_TASK_DRDY_TIMEOUT_MS);
            if((flags & SENSOR_TASK_FLAG_MPU_INT) != 0U) {
                if(Sensor_Task_RefreshMpuSample() != 0U) {
                    if(Sensor_Task_IsMotionDetected() != 0U) {
                        sensor_task_last_motion_ms = osKernelGetTickCount();
                    }
                }
            }

            if((osKernelGetTickCount() - sensor_task_last_env_refresh_ms) >= SENSOR_TASK_ENV_REFRESH_PERIOD_MS) {
                Sensor_Task_RefreshEnvironment();
            }

            if((osKernelGetTickCount() - sensor_task_last_motion_ms) >= SENSOR_TASK_STATIC_TO_SLEEP_MS) {
                Sensor_Task_SwitchToLowRateMode();
            }
        } else {
            Sensor_Task_RefreshEnvironment();
            if(Sensor_Task_RefreshMpuSample() != 0U) {
                if(Sensor_Task_IsMotionDetected() != 0U) {
                    sensor_task_last_motion_ms = osKernelGetTickCount();
                    Sensor_Task_SwitchToDataReadyMode();
                }
            }

            osDelay(SENSOR_TASK_LOW_RATE_PERIOD_MS);
        }
    }
}

/**
 * @brief 在 STOP 唤醒后执行一次抬腕判定。
 *
 * @param window_ms 抬腕观察窗口，单位毫秒。
 *
 * @return 1 表示抬腕成立；0 表示判定失败。
 */
uint8_t Sensor_Task_EvaluateRaiseWake(uint32_t window_ms)
{
    HwAccess_Vector3i16_t base_accel_mg;
    HwAccess_Vector3i16_t accel_mg;
    HwAccess_Vector3i16_t gyro_x10_dps;
    uint32_t start_ms;
    uint8_t matched_count = 0U;

    // ! 抬腕判定依赖角速度阈值，必须先恢复常规采样模式，避免低功耗唤醒阶段的陀螺仪待机影响结果。
    Sensor_Task_SwitchToDataReadyMode();

    if(Sensor_Task_RefreshMpuSample() == 0U) {
        return 0U;
    }

    if((HwAccess.mpu6050.get_accel_mg == NULL) ||
       (HwAccess.mpu6050.get_gyro_x10_dps == NULL) ||
       (HwAccess.mpu6050.get_accel_mg(&base_accel_mg) != 0)) {
        return 0U;
    }

    start_ms = osKernelGetTickCount();
    while((osKernelGetTickCount() - start_ms) < window_ms) {
        osDelay(SENSOR_TASK_RAISE_SAMPLE_PERIOD_MS);

        if(Sensor_Task_RefreshMpuSample() == 0U) {
            continue;
        }

        if((HwAccess.mpu6050.get_accel_mg(&accel_mg) != 0) ||
           (HwAccess.mpu6050.get_gyro_x10_dps(&gyro_x10_dps) != 0)) {
            continue;
        }

        if(Sensor_Task_IsRaiseSampleMatched(&base_accel_mg, &accel_mg, &gyro_x10_dps) != 0U) {
            matched_count++;
            if(matched_count >= SENSOR_TASK_RAISE_CONFIRM_COUNT) {
                Sensor_Task_SwitchToDataReadyMode();
                (void)Sensor_Task_RefreshMpuSample();
                sensor_task_last_motion_ms = osKernelGetTickCount();
                return 1U;
            }
        }
    }

    Sensor_Task_SwitchToLowRateMode();
    return 0U;
}

/**
 * @brief 在明确接受一次唤醒后，强制恢复活动采样模式。
 */
void Sensor_Task_ForceActiveMode(void)
{
    Sensor_Task_SwitchToDataReadyMode();
    (void)Sensor_Task_RefreshMpuSample();
    sensor_task_last_motion_ms = osKernelGetTickCount();
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
    if(HwAccess.power.update_battery_cache != NULL) {
        HwAccess.power.update_battery_cache();
    }

    if(HwAccess.aht21.update_cache != NULL) {
        (void)HwAccess.aht21.update_cache();
    }

    sensor_task_last_env_refresh_ms = osKernelGetTickCount();
}

/**
 * @brief 刷新 MPU6050 缓存，并保存运动判定输入。
 *
 * @return 1 表示成功刷新出有效样本；0 表示读取失败。
 */
static uint8_t Sensor_Task_RefreshMpuSample(void)
{
    HwAccess_Vector3i16_t accel_mg;
    HwAccess_Vector3i16_t gyro_x10_dps;

    if(HwAccess.mpu6050.update_cache != NULL) {
        (void)HwAccess.mpu6050.update_cache();
    }

    if((HwAccess.mpu6050.get_accel_mg == NULL) ||
       (HwAccess.mpu6050.get_gyro_x10_dps == NULL) ||
       (HwAccess.mpu6050.get_accel_mg(&accel_mg) != 0) ||
       (HwAccess.mpu6050.get_gyro_x10_dps(&gyro_x10_dps) != 0)) {
        return 0U;
    }

    sensor_task_current_accel_mg = accel_mg;
    sensor_task_current_gyro_x10_dps = gyro_x10_dps;
    return 1U;
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
    HwAccess_Vector3i16_t accel_mg = sensor_task_current_accel_mg;
    HwAccess_Vector3i16_t gyro_x10_dps = sensor_task_current_gyro_x10_dps;
    int16_t accel_dx;
    int16_t accel_dy;
    int16_t accel_dz;

    if(sensor_task_motion_baseline_valid == 0U) {
        sensor_task_motion_baseline_valid = 1U;
        sensor_task_last_accel_mg = accel_mg;
        sensor_task_last_gyro_x10_dps = gyro_x10_dps;
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
 * @brief 判断单个采样点是否满足抬腕阈值。
 */
static uint8_t Sensor_Task_IsRaiseSampleMatched(const HwAccess_Vector3i16_t * base_accel,
                                                const HwAccess_Vector3i16_t * accel_mg,
                                                const HwAccess_Vector3i16_t * gyro_x10_dps)
{
    int16_t delta_y;
    int16_t delta_z;

    if((base_accel == NULL) || (accel_mg == NULL) || (gyro_x10_dps == NULL)) {
        return 0U;
    }

    delta_y = Sensor_Task_AbsI16((int16_t)(accel_mg->y - base_accel->y));
    delta_z = Sensor_Task_AbsI16((int16_t)(accel_mg->z - base_accel->z));

    if((delta_y < SENSOR_TASK_RAISE_ACCEL_Y_DELTA_MG) &&
       (delta_z < SENSOR_TASK_RAISE_ACCEL_Z_DELTA_MG)) {
        return 0U;
    }

    if((Sensor_Task_AbsI16(gyro_x10_dps->x) < SENSOR_TASK_RAISE_GYRO_X10_DPS) &&
       (Sensor_Task_AbsI16(gyro_x10_dps->y) < SENSOR_TASK_RAISE_GYRO_X10_DPS) &&
       (Sensor_Task_AbsI16(gyro_x10_dps->z) < SENSOR_TASK_RAISE_GYRO_X10_DPS)) {
        return 0U;
    }

    return 1U;
}

/**
 * @brief 取 int16_t 绝对值。
 */
static int16_t Sensor_Task_AbsI16(int16_t value)
{
    return (value >= 0) ? value : (int16_t)(-value);
}
