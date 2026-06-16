/**
 * @file bsp_mpu6050.c
 * @brief MPU6050 六轴 IMU 驱动。
 */

#include "bsp_mpu6050.h"

#include <stddef.h>

#include "bsp_iic.h"
#include "stm32f4xx_hal.h"

#define MPU6050_REG_SMPLRT_DIV      0x19U
#define MPU6050_REG_CONFIG          0x1AU
#define MPU6050_REG_GYRO_CONFIG     0x1BU
#define MPU6050_REG_ACCEL_CONFIG    0x1CU
#define MPU6050_REG_ACCEL_XOUT_H    0x3BU
#define MPU6050_REG_PWR_MGMT_1      0x6BU
#define MPU6050_REG_PWR_MGMT_2      0x6CU
#define MPU6050_REG_WHO_AM_I        0x75U

#define MPU6050_WHO_AM_I_VALUE      0x68U
#define MPU6050_PWR_RESET           0x80U
#define MPU6050_PWR_CLK_PLL_XGYRO   0x01U
#define MPU6050_ALL_AXES_ENABLE     0x00U
#define MPU6050_DLPF_CFG_44HZ       0x03U
#define MPU6050_SAMPLE_DIV_100HZ    9U
#define MPU6050_GYRO_FS_250DPS      0x00U
#define MPU6050_ACCEL_FS_2G         0x00U

#define MPU6050_RESET_DELAY_MS      100U
#define MPU6050_WAKE_DELAY_MS       10U
#define MPU6050_ACCEL_LSB_PER_G     16384L
#define MPU6050_GYRO_LSB_PER_DPS    131L

static uint8_t mpu6050_initialized;
static uint8_t mpu6050_addr = BSP_MPU6050_ADDR_LOW_7BIT;

static int BSP_MPU6050_DetectAddress(void);
static int BSP_MPU6050_ReadWhoAmI(uint8_t addr, uint8_t * value);
static int16_t BSP_MPU6050_CombineBe(uint8_t high, uint8_t low);
static int16_t BSP_MPU6050_ScaleAccelMg(int16_t raw);
static int16_t BSP_MPU6050_ScaleGyroX10Dps(int16_t raw);
static int16_t BSP_MPU6050_ScaleTemperatureX10C(int16_t raw);

/**
 * @brief 初始化 MPU6050 并配置为 +/-2g、+/-250dps、约 100Hz 采样。
 */
int BSP_MPU6050_Init(void)
{
    int ret;

    BSP_IIC_Init();

    ret = BSP_MPU6050_DetectAddress();
    if(ret != 0) {
        return ret;
    }

    // 先触发器件复位，避免上一次调试留下睡眠、待机或非默认量程配置。
    if(BSP_IIC_WriteReg(mpu6050_addr, MPU6050_REG_PWR_MGMT_1, MPU6050_PWR_RESET) != 0) {
        return -2;
    }
    HAL_Delay(MPU6050_RESET_DELAY_MS);

    ret = BSP_MPU6050_DetectAddress();
    if(ret != 0) {
        return ret;
    }

    // 唤醒芯片，并选择 X 轴陀螺仪 PLL 作为时钟源，减少内部 RC 时钟漂移影响。
    if(BSP_IIC_WriteReg(mpu6050_addr, MPU6050_REG_PWR_MGMT_1, MPU6050_PWR_CLK_PLL_XGYRO) != 0) {
        return -3;
    }
    HAL_Delay(MPU6050_WAKE_DELAY_MS);

    if(BSP_IIC_WriteReg(mpu6050_addr, MPU6050_REG_PWR_MGMT_2, MPU6050_ALL_AXES_ENABLE) != 0) {
        return -4;
    }

    if(BSP_IIC_WriteReg(mpu6050_addr, MPU6050_REG_CONFIG, MPU6050_DLPF_CFG_44HZ) != 0) {
        return -5;
    }

    if(BSP_IIC_WriteReg(mpu6050_addr, MPU6050_REG_SMPLRT_DIV, MPU6050_SAMPLE_DIV_100HZ) != 0) {
        return -6;
    }

    if(BSP_IIC_WriteReg(mpu6050_addr, MPU6050_REG_GYRO_CONFIG, MPU6050_GYRO_FS_250DPS) != 0) {
        return -7;
    }

    if(BSP_IIC_WriteReg(mpu6050_addr, MPU6050_REG_ACCEL_CONFIG, MPU6050_ACCEL_FS_2G) != 0) {
        return -8;
    }

    mpu6050_initialized = 1U;
    return 0;
}

/**
 * @brief 探测 MPU6050 0x68/0x69 地址并校验 WHO_AM_I。
 */
int BSP_MPU6050_Probe(void)
{
    BSP_IIC_Init();
    return BSP_MPU6050_DetectAddress();
}

/**
 * @brief 连续读取 MPU6050 14 字节采样寄存器并换算工程单位。
 */
int BSP_MPU6050_Read(BSP_MPU6050_Data_t * data)
{
    uint8_t buf[14];

    if(data == NULL) {
        return -1;
    }

    if(mpu6050_initialized == 0U) {
        int ret = BSP_MPU6050_Init();
        if(ret != 0) {
            return ret;
        }
    }

    if(BSP_IIC_ReadRegs(mpu6050_addr, MPU6050_REG_ACCEL_XOUT_H, buf, sizeof(buf)) != 0) {
        return -2;
    }

    // MPU6050 输出寄存器为大端格式，顺序为加速度、温度、陀螺仪。
    data->accel_raw.x = BSP_MPU6050_CombineBe(buf[0], buf[1]);
    data->accel_raw.y = BSP_MPU6050_CombineBe(buf[2], buf[3]);
    data->accel_raw.z = BSP_MPU6050_CombineBe(buf[4], buf[5]);
    data->temperature_raw = BSP_MPU6050_CombineBe(buf[6], buf[7]);
    data->gyro_raw.x = BSP_MPU6050_CombineBe(buf[8], buf[9]);
    data->gyro_raw.y = BSP_MPU6050_CombineBe(buf[10], buf[11]);
    data->gyro_raw.z = BSP_MPU6050_CombineBe(buf[12], buf[13]);

    data->accel_mg.x = BSP_MPU6050_ScaleAccelMg(data->accel_raw.x);
    data->accel_mg.y = BSP_MPU6050_ScaleAccelMg(data->accel_raw.y);
    data->accel_mg.z = BSP_MPU6050_ScaleAccelMg(data->accel_raw.z);
    data->gyro_x10_dps.x = BSP_MPU6050_ScaleGyroX10Dps(data->gyro_raw.x);
    data->gyro_x10_dps.y = BSP_MPU6050_ScaleGyroX10Dps(data->gyro_raw.y);
    data->gyro_x10_dps.z = BSP_MPU6050_ScaleGyroX10Dps(data->gyro_raw.z);
    data->temperature_x10_c = BSP_MPU6050_ScaleTemperatureX10C(data->temperature_raw);

    return 0;
}

/**
 * @brief 自动探测 AD0 对应的两个 7 位地址。
 */
static int BSP_MPU6050_DetectAddress(void)
{
    uint8_t id;

    if((BSP_MPU6050_ReadWhoAmI(BSP_MPU6050_ADDR_LOW_7BIT, &id) == 0) &&
       (id == MPU6050_WHO_AM_I_VALUE)) {
        mpu6050_addr = BSP_MPU6050_ADDR_LOW_7BIT;
        return 0;
    }

    if((BSP_MPU6050_ReadWhoAmI(BSP_MPU6050_ADDR_HIGH_7BIT, &id) == 0) &&
       (id == MPU6050_WHO_AM_I_VALUE)) {
        mpu6050_addr = BSP_MPU6050_ADDR_HIGH_7BIT;
        return 0;
    }

    return -1;
}

/**
 * @brief 读取 MPU6050 WHO_AM_I 寄存器。
 */
static int BSP_MPU6050_ReadWhoAmI(uint8_t addr, uint8_t * value)
{
    if(value == NULL) {
        return -1;
    }

    return BSP_IIC_ReadRegs(addr, MPU6050_REG_WHO_AM_I, value, 1U);
}

/**
 * @brief 合成大端 16 位有符号数。
 */
static int16_t BSP_MPU6050_CombineBe(uint8_t high, uint8_t low)
{
    return (int16_t)(((uint16_t)high << 8) | (uint16_t)low);
}

/**
 * @brief 将 +/-2g 原始计数换算为 mg。
 */
static int16_t BSP_MPU6050_ScaleAccelMg(int16_t raw)
{
    return (int16_t)(((int32_t)raw * 1000L) / MPU6050_ACCEL_LSB_PER_G);
}

/**
 * @brief 将 +/-250dps 原始计数换算为 0.1dps。
 */
static int16_t BSP_MPU6050_ScaleGyroX10Dps(int16_t raw)
{
    return (int16_t)(((int32_t)raw * 10L) / MPU6050_GYRO_LSB_PER_DPS);
}

/**
 * @brief 将温度原始计数换算为 0.1 摄氏度。
 */
static int16_t BSP_MPU6050_ScaleTemperatureX10C(int16_t raw)
{
    return (int16_t)((((int32_t)raw * 10L) / 340L) + 365L);
}
