#include "hwaccess.h"

#include "bsp_aht21.h"
#include "bsp_iic.h"
#include "bsp_em7028.h"
#include "bsp_ext_watchdog.h"
#include "bsp_bluetooth.h"
#include "bsp_key.h"
#include "bsp_mpu6050.h"
#include "bsp_power.h"
#include "bsp_prom.h"
#include "CST816T.h"
#include "main.h"
#include "st7789v.h"

#define HWACCESS_BATTERY_MIN_MV 2750U
#define HWACCESS_BATTERY_MAX_MV 4200U
#define HWACCESS_STEP_BASELINE_SHIFT 3U
#define HWACCESS_STEP_HIGH_DELTA_MG2 350000UL
#define HWACCESS_STEP_LOW_DELTA_MG2 120000UL
#define HWACCESS_STEP_REFRACTORY_SAMPLES 1U
static void HwAccess_Lcd_Init(void);
static uint8_t HwAccess_Key_IsPressed(HwAccess_KeyId_t key);
static void HwAccess_Power_UpdateBatteryCache(void);
static uint16_t HwAccess_Power_GetBatteryVoltageMv(void);
static uint8_t HwAccess_Power_GetBatteryPercent(void);
static uint8_t HwAccess_Power_IsBatteryValid(void);
static int HwAccess_Aht21_UpdateCache(void);
static int16_t HwAccess_Aht21_GetTemperatureX10C(void);
static uint16_t HwAccess_Aht21_GetHumidityX10Percent(void);
static uint8_t HwAccess_Aht21_IsValid(void);
static int HwAccess_Mpu6050_UpdateCache(void);
static int HwAccess_Mpu6050_GetAccelMg(HwAccess_Vector3i16_t * value);
static int HwAccess_Mpu6050_GetGyroX10Dps(HwAccess_Vector3i16_t * value);
static int16_t HwAccess_Mpu6050_GetTemperatureX10C(void);
static uint32_t HwAccess_Mpu6050_GetStepCount(void);
static void HwAccess_Mpu6050_ResetStepCount(void);
static uint8_t HwAccess_Mpu6050_IsValid(void);
static void HwAccess_Mpu6050_UpdateStepCounter(const BSP_MPU6050_Data_t * data);
static uint32_t HwAccess_Mpu6050_AccelMag2(const BSP_MPU6050_Vector_t * accel);
static int HwAccess_Em7028_Start(void);
static int HwAccess_Em7028_Stop(void);
static int HwAccess_Em7028_ReadRaw(uint16_t * value);
static int HwAccess_Em7028_UpdateCache(void);
static int HwAccess_Em7028_GetProbeStatus(void);
static int HwAccess_Em7028_ReadReg(uint8_t reg, uint8_t * value);
static uint8_t HwAccess_Em7028_GetPid(void);
static int HwAccess_Em7028_GetLastI2cStatus(void);
static uint32_t HwAccess_Em7028_GetLastI2cError(void);
static uint16_t HwAccess_Em7028_GetRaw(void);
static uint8_t HwAccess_Em7028_GetBpm(void);
static uint8_t HwAccess_Em7028_IsValid(void);
static uint8_t HwAccess_Em7028_IsRunning(void);
static void HwAccess_Em7028_ResetState(void);

static volatile uint16_t hwaccess_battery_voltage_mv;
static volatile uint8_t hwaccess_battery_percent;
static volatile uint8_t hwaccess_battery_valid;
static volatile int16_t hwaccess_aht21_temperature_x10_c;
static volatile uint16_t hwaccess_aht21_humidity_x10_percent;
static volatile uint8_t hwaccess_aht21_valid;
static volatile int16_t hwaccess_mpu6050_accel_mg_x;
static volatile int16_t hwaccess_mpu6050_accel_mg_y;
static volatile int16_t hwaccess_mpu6050_accel_mg_z;
static volatile int16_t hwaccess_mpu6050_gyro_x10_dps_x;
static volatile int16_t hwaccess_mpu6050_gyro_x10_dps_y;
static volatile int16_t hwaccess_mpu6050_gyro_x10_dps_z;
static volatile int16_t hwaccess_mpu6050_temperature_x10_c;
static volatile uint8_t hwaccess_mpu6050_valid;
static volatile uint32_t hwaccess_mpu6050_step_count;
static uint32_t hwaccess_mpu6050_step_baseline_mg2;
static uint8_t hwaccess_mpu6050_step_baseline_valid;
static uint8_t hwaccess_mpu6050_step_high_state;
static uint8_t hwaccess_mpu6050_step_refractory;
static volatile uint16_t hwaccess_em7028_raw;
static volatile uint8_t hwaccess_em7028_bpm;
static volatile uint8_t hwaccess_em7028_valid;
static volatile uint8_t hwaccess_em7028_running;

/**
 * @brief 初始化屏幕相关硬件。
 *
 * LCD 和触摸共同组成一块屏幕输入/输出设备，启动任务只需要调用这一处入口。
 */
static void HwAccess_Lcd_Init(void)
{
    st7789_Init();

    // 触摸初始化依赖虚拟 I2C 和复位脚，留在硬件访问层内统一封装。
    (void)CST816T_Init();
}

/**
 * @brief 读取指定按键是否处于按下状态。
 *
 * @param key 按键逻辑编号。
 * @return 1 表示按下，0 表示未按下或编号无效。
 */
static uint8_t HwAccess_Key_IsPressed(HwAccess_KeyId_t key)
{
    switch(key) {
        case HWACCESS_KEY_BACK:
            return BSP_Key_IsPressed(BSP_KEY_BACK);

        case HWACCESS_KEY_SCREEN:
            return BSP_Key_IsPressed(BSP_KEY_SCREEN);

        default:
            return 0U;
    }
}

/**
 * @brief 采样一次电池电压，并把电压和百分比写入 HwAccess 缓存。
 *
 * 该函数由普通任务调用，内部通过 BSP Power 完成 PA1 ADC 读取；UI 页面只读取缓存，
 * 不在 LVGL 刷新路径里触发 ADC 轮询。
 */
static void HwAccess_Power_UpdateBatteryCache(void)
{
    uint16_t voltage_mv = BSP_Power_ReadBatteryVoltageMv();
    uint8_t percent;

    if(voltage_mv == 0U) {
        return;
    }

    if(voltage_mv <= HWACCESS_BATTERY_MIN_MV) {
        percent = 0U;
    } else if(voltage_mv >= HWACCESS_BATTERY_MAX_MV) {
        percent = 100U;
    } else {
        percent = (uint8_t)(((uint32_t)(voltage_mv - HWACCESS_BATTERY_MIN_MV) * 100U) /
                            (HWACCESS_BATTERY_MAX_MV - HWACCESS_BATTERY_MIN_MV));
    }

    hwaccess_battery_voltage_mv = voltage_mv;
    hwaccess_battery_percent = percent;
    hwaccess_battery_valid = 1U;
}

/**
 * @brief 从缓存读取最近一次有效电池电压。
 * @return 电池端电压，单位 mV；尚未采样成功时返回 0。
 */
static uint16_t HwAccess_Power_GetBatteryVoltageMv(void)
{
    return (hwaccess_battery_valid != 0U) ? hwaccess_battery_voltage_mv : 0U;
}

/**
 * @brief 从缓存读取最近一次电池电量百分比。
 * @return 电量百分比，范围 0~100；尚未采样成功时返回 0。
 */
static uint8_t HwAccess_Power_GetBatteryPercent(void)
{
    return (hwaccess_battery_valid != 0U) ? hwaccess_battery_percent : 0U;
}

/**
 * @brief 查询电池缓存是否已有有效采样。
 * @return 1 表示缓存有效，0 表示尚未成功采样。
 */
static uint8_t HwAccess_Power_IsBatteryValid(void)
{
    return hwaccess_battery_valid;
}

/**
 * @brief 采样一次 AHT21 并刷新温湿度缓存。
 *
 * 该函数运行在 Sensor_Task 上下文，允许等待 AHT21 转换完成；UI 只读缓存，
 * 不在 LVGL 刷新路径里触发软件 I2C 时序。
 */
static int HwAccess_Aht21_UpdateCache(void)
{
    BSP_AHT21_Data_t data;
    int ret = BSP_AHT21_Read(&data);

    if(ret != 0) {
        return ret;
    }

    hwaccess_aht21_temperature_x10_c = data.temperature_x10_c;
    hwaccess_aht21_humidity_x10_percent = data.humidity_x10_percent;
    hwaccess_aht21_valid = 1U;

    return 0;
}

/**
 * @brief 从缓存读取最近一次 AHT21 温度。
 */
static int16_t HwAccess_Aht21_GetTemperatureX10C(void)
{
    return (hwaccess_aht21_valid != 0U) ? hwaccess_aht21_temperature_x10_c : 0;
}

/**
 * @brief 从缓存读取最近一次 AHT21 相对湿度。
 */
static uint16_t HwAccess_Aht21_GetHumidityX10Percent(void)
{
    return (hwaccess_aht21_valid != 0U) ? hwaccess_aht21_humidity_x10_percent : 0U;
}

/**
 * @brief 查询 AHT21 缓存是否已有有效采样。
 */
static uint8_t HwAccess_Aht21_IsValid(void)
{
    return hwaccess_aht21_valid;
}

/**
 * @brief 全局硬件访问对象。
 *
 * 该表把任务层可用的硬件接口绑定到具体 BSP 实现，避免任务直接依赖底层模块。
 */
/**
 * @brief 采样一次 LSM303DLHC 并刷新三轴缓存。
 *
 * 该函数运行在 Sensor_Task 上下文，允许通过软件 I2C 阻塞读取；UI 和业务层只读取缓存。
 */

/**
 * @brief 读取缓存加速度，单位 mg。
 */

/**
 * @brief 读取缓存磁场，单位毫高斯。
 */

/**
 * @brief 查询 LSM303DLHC 缓存是否已有有效采样。
 */

/**
 * @brief 采样一次 MPU6050 并刷新六轴与温度缓存。
 *
 * 该函数运行在 Sensor_Task 上下文，允许通过软件 I2C 阻塞读取；UI 和业务层只读取缓存。
 */
static int HwAccess_Mpu6050_UpdateCache(void)
{
    BSP_MPU6050_Data_t data;
    int ret = BSP_MPU6050_Read(&data);

    if(ret != 0) {
        return ret;
    }

    hwaccess_mpu6050_accel_mg_x = data.accel_mg.x;
    hwaccess_mpu6050_accel_mg_y = data.accel_mg.y;
    hwaccess_mpu6050_accel_mg_z = data.accel_mg.z;
    hwaccess_mpu6050_gyro_x10_dps_x = data.gyro_x10_dps.x;
    hwaccess_mpu6050_gyro_x10_dps_y = data.gyro_x10_dps.y;
    hwaccess_mpu6050_gyro_x10_dps_z = data.gyro_x10_dps.z;
    hwaccess_mpu6050_temperature_x10_c = data.temperature_x10_c;
    HwAccess_Mpu6050_UpdateStepCounter(&data);
    hwaccess_mpu6050_valid = 1U;

    return 0;
}

/**
 * @brief 读取缓存加速度，单位 mg。
 */
static int HwAccess_Mpu6050_GetAccelMg(HwAccess_Vector3i16_t * value)
{
    if(value == NULL) {
        return -1;
    }

    if(hwaccess_mpu6050_valid == 0U) {
        return -2;
    }

    value->x = hwaccess_mpu6050_accel_mg_x;
    value->y = hwaccess_mpu6050_accel_mg_y;
    value->z = hwaccess_mpu6050_accel_mg_z;

    return 0;
}

/**
 * @brief 读取缓存角速度，单位 0.1dps。
 */
static int HwAccess_Mpu6050_GetGyroX10Dps(HwAccess_Vector3i16_t * value)
{
    if(value == NULL) {
        return -1;
    }

    if(hwaccess_mpu6050_valid == 0U) {
        return -2;
    }

    value->x = hwaccess_mpu6050_gyro_x10_dps_x;
    value->y = hwaccess_mpu6050_gyro_x10_dps_y;
    value->z = hwaccess_mpu6050_gyro_x10_dps_z;

    return 0;
}

/**
 * @brief 读取缓存温度，单位 0.1 摄氏度。
 */
static int16_t HwAccess_Mpu6050_GetTemperatureX10C(void)
{
    return (hwaccess_mpu6050_valid != 0U) ? hwaccess_mpu6050_temperature_x10_c : 0;
}

/**
 * @brief 查询 MPU6050 缓存是否已有有效采样。
 */
/**
 * @brief 读取 MPU6050 计步估算值。
 */
static uint32_t HwAccess_Mpu6050_GetStepCount(void)
{
    return hwaccess_mpu6050_step_count;
}

/**
 * @brief 清零 MPU6050 计步状态。
 */
static void HwAccess_Mpu6050_ResetStepCount(void)
{
    hwaccess_mpu6050_step_count = 0U;
    hwaccess_mpu6050_step_baseline_mg2 = 0U;
    hwaccess_mpu6050_step_baseline_valid = 0U;
    hwaccess_mpu6050_step_high_state = 0U;
    hwaccess_mpu6050_step_refractory = 0U;
}

static uint8_t HwAccess_Mpu6050_IsValid(void)
{
    return hwaccess_mpu6050_valid;
}

/**
 * @brief 使用加速度模长变化估算步数。
 *
 * Sensor_Task 当前周期为 500ms，因此这里使用低频友好的阈值和峰值锁存：
 * 模长平方明显高于动态基线时计一步，回落到低阈值后才允许下一次计步。
 */
static void HwAccess_Mpu6050_UpdateStepCounter(const BSP_MPU6050_Data_t * data)
{
    uint32_t mag2;
    uint32_t delta;
    int32_t baseline_diff;

    if(data == NULL) {
        return;
    }

    mag2 = HwAccess_Mpu6050_AccelMag2(&data->accel_mg);

    if(hwaccess_mpu6050_step_baseline_valid == 0U) {
        hwaccess_mpu6050_step_baseline_mg2 = mag2;
        hwaccess_mpu6050_step_baseline_valid = 1U;
        return;
    }

    baseline_diff = (int32_t)mag2 - (int32_t)hwaccess_mpu6050_step_baseline_mg2;
    hwaccess_mpu6050_step_baseline_mg2 =
        (uint32_t)((int32_t)hwaccess_mpu6050_step_baseline_mg2 +
                   (baseline_diff >> HWACCESS_STEP_BASELINE_SHIFT));

    if(hwaccess_mpu6050_step_refractory > 0U) {
        hwaccess_mpu6050_step_refractory--;
    }

    delta = (mag2 > hwaccess_mpu6050_step_baseline_mg2) ?
            (mag2 - hwaccess_mpu6050_step_baseline_mg2) :
            0U;

    if((hwaccess_mpu6050_step_high_state == 0U) &&
       (hwaccess_mpu6050_step_refractory == 0U) &&
       (delta >= HWACCESS_STEP_HIGH_DELTA_MG2)) {
        hwaccess_mpu6050_step_count++;
        hwaccess_mpu6050_step_high_state = 1U;
        hwaccess_mpu6050_step_refractory = HWACCESS_STEP_REFRACTORY_SAMPLES;
    } else if((hwaccess_mpu6050_step_high_state != 0U) &&
              (delta <= HWACCESS_STEP_LOW_DELTA_MG2)) {
        hwaccess_mpu6050_step_high_state = 0U;
    }
}

/**
 * @brief 计算加速度 mg 向量的模长平方，避免在计步路径中使用 sqrt()。
 */
static uint32_t HwAccess_Mpu6050_AccelMag2(const BSP_MPU6050_Vector_t * accel)
{
    int32_t x;
    int32_t y;
    int32_t z;

    if(accel == NULL) {
        return 0U;
    }

    x = accel->x;
    y = accel->y;
    z = accel->z;

    return (uint32_t)((x * x) + (y * y) + (z * z));
}

/**
 * @brief 启动 EM7028 心率监测并重置算法状态。
 */
static int HwAccess_Em7028_Start(void)
{
    int ret = BSP_EM7028_Start();

    if(ret != 0) {
        hwaccess_em7028_running = 0U;
        return ret;
    }

    HwAccess_Em7028_ResetState();
    hwaccess_em7028_running = 1U;
    return 0;
}

/**
 * @brief 停止 EM7028 心率监测。
 */
static int HwAccess_Em7028_Stop(void)
{
    hwaccess_em7028_running = 0U;
    return BSP_EM7028_Stop();
}

/**
 * @brief 读取一次 EM7028 原始值，并同步更新 HwAccess 缓存。
 */
static int HwAccess_Em7028_ReadRaw(uint16_t * value)
{
    uint16_t raw_value = 0U;
    int ret;

    if((value == NULL) || (hwaccess_em7028_running == 0U)) {
        return -2;
    }

    ret = BSP_EM7028_ReadHrs1Data(&raw_value);
    if(ret != 0) {
        return ret;
    }

    hwaccess_em7028_raw = raw_value;
    hwaccess_em7028_valid = 1U;
    *value = raw_value;
    return 0;
}

/**
 * @brief 读取一次 EM7028 原始值并刷新原始 PPG 缓存。
 */
static int HwAccess_Em7028_UpdateCache(void)
{
    uint16_t raw_value = 0U;

    return HwAccess_Em7028_ReadRaw(&raw_value);
}

/**
 * @brief 获取最近一次 EM7028 地址探测结果。
 */
static int HwAccess_Em7028_GetProbeStatus(void)
{
    return BSP_EM7028_GetLastProbeStatus();
}

/**
 * @brief 读取一个 EM7028 寄存器原始字节。
 */
static int HwAccess_Em7028_ReadReg(uint8_t reg, uint8_t * value)
{
    return BSP_EM7028_ReadRegister(reg, value);
}

/**
 * @brief 获取最近一次读到的 EM7028 PID 原始字节。
 */
static uint8_t HwAccess_Em7028_GetPid(void)
{
    return BSP_EM7028_GetLastPid();
}

/**
 * @brief 获取最近一次 EM7028 访问对应的软件 IIC HAL 状态。
 */
static int HwAccess_Em7028_GetLastI2cStatus(void)
{
    return (int)BSP_IIC_GetLastHalStatus();
}

/**
 * @brief 获取最近一次 EM7028 访问对应的软件 IIC HAL 错误码。
 */
static uint32_t HwAccess_Em7028_GetLastI2cError(void)
{
    return BSP_IIC_GetLastHalError();
}

/**
 * @brief 获取最近一次 EM7028 原始 ADC 值。
 */
static uint16_t HwAccess_Em7028_GetRaw(void)
{
    return hwaccess_em7028_raw;
}

/**
 * @brief 获取最近一次心率值接口占位。
 *
 * 当前版本只输出原始 PPG 数据，不进行 BPM 计算。
 */
static uint8_t HwAccess_Em7028_GetBpm(void)
{
    return hwaccess_em7028_bpm;
}

/**
 * @brief 判断 EM7028 心率缓存是否有效。
 */
static uint8_t HwAccess_Em7028_IsValid(void)
{
    return hwaccess_em7028_valid;
}

/**
 * @brief 判断当前是否正在执行 EM7028 心率监测。
 */
static uint8_t HwAccess_Em7028_IsRunning(void)
{
    return hwaccess_em7028_running;
}

/**
 * @brief 清空 EM7028 原始 PPG 缓存状态。
 */
static void HwAccess_Em7028_ResetState(void)
{
    hwaccess_em7028_raw = 0U;
    hwaccess_em7028_bpm = 0U;
    hwaccess_em7028_valid = 0U;
}

/**
 * @brief 记录一次原始 PPG 采样已更新。
 *
 * 当前版本只负责保留 EM7028 的原始 ADC/PPG 值，
 * 不做峰值检测、滤波或 BPM 解算。
 */
void HwAccess_Em7028_UpdateRawCache(uint16_t raw_ppg, uint8_t raw_valid)
{
    hwaccess_em7028_raw = raw_ppg;
    hwaccess_em7028_valid = raw_valid;
}

/**
 * @brief 记录一次有效 RR 间期，并更新平滑后的 BPM。
 *
 * 这里用固定长度环形缓冲保存最近几次心跳间隔，避免单个峰值误差让 BPM 大幅抖动。
 */
void HwAccess_Em7028_UpdateHeartRateCache(uint8_t bpm, uint8_t hr_valid)
{
    hwaccess_em7028_bpm = (hr_valid != 0U) ? bpm : 0U;
}

obj_HwAccess HwAccess = {
    .lcd = {
        .init = HwAccess_Lcd_Init,
        .deinit = st7789_DeInit,
        .set_backlight = st7789_SetBacklight,
        .display_on = st7789_DisplayOn,
        .display_off = st7789_DisplayOff,
    },
    .key = {
        .init = BSP_Key_Init,
        .is_pressed = HwAccess_Key_IsPressed,
    },
    .power = {
        .open = BSP_Power_Open,
        .close = BSP_Power_Close,
        .is_charging = BSP_Power_IsCharging,
        .update_battery_cache = HwAccess_Power_UpdateBatteryCache,
        .get_battery_voltage_mv = HwAccess_Power_GetBatteryVoltageMv,
        .get_battery_percent = HwAccess_Power_GetBatteryPercent,
        .is_battery_valid = HwAccess_Power_IsBatteryValid,
    },
    .watchdog = {
        .init = BSP_ExtWatchdog_Init,
        .enable = BSP_ExtWatchdog_Enable,
        .disable = BSP_ExtWatchdog_Disable,
        .feed = BSP_ExtWatchdog_Feed,
    },
    .prom = {
        .init = BSP_PROM_Init,
        .probe = BSP_PROM_Probe,
        .read = BSP_PROM_Read,
        .write = BSP_PROM_Write,
        .read_byte = BSP_PROM_ReadByte,
        .write_byte = BSP_PROM_WriteByte,
    },
    .bluetooth = {
        .init = BSP_BlueTooth_Init,
        .enable = BSP_BlueTooth_Enable,
        .disable = BSP_BlueTooth_Disable,
        .is_enabled = BSP_BlueTooth_IsEnabled,
        .send = BSP_BlueTooth_Send,
        .send_dma = BSP_BlueTooth_SendDma,
        .is_tx_busy = BSP_BlueTooth_IsTxBusy,
        .take_tx_done = BSP_BlueTooth_TakeTxDone,
        .register_tx_complete_hook = BSP_BlueTooth_RegisterTxCompleteHook,
        .register_error_hook = BSP_BlueTooth_RegisterErrorHook,
        .receive = BSP_BlueTooth_Receive,
    },
    .aht21 = {
        .init = BSP_AHT21_Init,
        .probe = BSP_AHT21_Probe,
        .update_cache = HwAccess_Aht21_UpdateCache,
        .get_temperature_x10_c = HwAccess_Aht21_GetTemperatureX10C,
        .get_humidity_x10_percent = HwAccess_Aht21_GetHumidityX10Percent,
        .is_valid = HwAccess_Aht21_IsValid,
    },
    .lsm303dlhc = { 0 },
    .mpu6050 = {
        .init = BSP_MPU6050_Init,
        .probe = BSP_MPU6050_Probe,
        .update_cache = HwAccess_Mpu6050_UpdateCache,
        .get_accel_mg = HwAccess_Mpu6050_GetAccelMg,
        .get_gyro_x10_dps = HwAccess_Mpu6050_GetGyroX10Dps,
        .get_temperature_x10_c = HwAccess_Mpu6050_GetTemperatureX10C,
        .get_step_count = HwAccess_Mpu6050_GetStepCount,
        .reset_step_count = HwAccess_Mpu6050_ResetStepCount,
        .is_valid = HwAccess_Mpu6050_IsValid,
    },
    .em7028 = {
        .init = BSP_EM7028_Init,
        .probe = BSP_EM7028_Probe,
        .start = HwAccess_Em7028_Start,
        .stop = HwAccess_Em7028_Stop,
        .read_raw = HwAccess_Em7028_ReadRaw,
        .update_cache = HwAccess_Em7028_UpdateCache,
        .get_probe_status = HwAccess_Em7028_GetProbeStatus,
        .read_reg = HwAccess_Em7028_ReadReg,
        .get_pid = HwAccess_Em7028_GetPid,
        .get_last_i2c_status = HwAccess_Em7028_GetLastI2cStatus,
        .get_last_i2c_error = HwAccess_Em7028_GetLastI2cError,
        .get_raw = HwAccess_Em7028_GetRaw,
        .get_bpm = HwAccess_Em7028_GetBpm,
        .is_valid = HwAccess_Em7028_IsValid,
        .is_running = HwAccess_Em7028_IsRunning,
    },
};
