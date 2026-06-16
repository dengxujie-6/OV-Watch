/**
 * @file bsp_lsm303dlhc.c
 * @brief LSM303DLHC 加速度计和磁力计驱动。
 */

#include "bsp_lsm303dlhc.h"

#include <stddef.h>

#include "bsp_iic.h"

#define LSM303DLHC_REG_WHO_AM_I_A       0x0FU
#define LSM303DLHC_WHO_AM_I_A_VALUE     0x33U
#define LSM303DLHC_REG_CTRL_REG1_A      0x20U
#define LSM303DLHC_REG_CTRL_REG4_A      0x23U
#define LSM303DLHC_REG_OUT_X_L_A        0x28U
#define LSM303DLHC_ACCEL_AUTO_INC       0x80U

#define LSM303DLHC_REG_CRA_REG_M        0x00U
#define LSM303DLHC_REG_CRB_REG_M        0x01U
#define LSM303DLHC_REG_MR_REG_M         0x02U
#define LSM303DLHC_REG_OUT_X_H_M        0x03U

#define LSM303DLHC_CTRL_REG1_A_100HZ_XYZ 0x57U
#define LSM303DLHC_CTRL_REG4_A_2G_HR_BDU 0x88U
#define LSM303DLHC_CRA_REG_M_15HZ        0x10U
#define LSM303DLHC_CRB_REG_M_1_3GAUSS    0x20U
#define LSM303DLHC_MR_REG_M_CONTINUOUS   0x00U

#define LSM303DLHC_MAG_XY_SENS_LSB_PER_GAUSS  1100L
#define LSM303DLHC_MAG_Z_SENS_LSB_PER_GAUSS   980L

static uint8_t lsm303dlhc_initialized;
static uint8_t lsm303dlhc_accel_addr = BSP_LSM303DLHC_ACCEL_ADDR_HIGH_7BIT;

static int BSP_LSM303DLHC_DetectAccelAddress(void);
static int BSP_LSM303DLHC_ReadAccelWhoAmI(uint8_t addr, uint8_t * value);
static int16_t BSP_LSM303DLHC_CombineLe(const uint8_t * data);
static int16_t BSP_LSM303DLHC_CombineBe(uint8_t high, uint8_t low);
static int16_t BSP_LSM303DLHC_ScaleMag(int16_t raw, int32_t sensitivity);

/**
 * @brief 初始化 LSM303DLHC 加速度计和磁力计。
 */
int BSP_LSM303DLHC_Init(void)
{
    int ret;

    BSP_IIC_Init();

    ret = BSP_LSM303DLHC_DetectAccelAddress();
    if(ret != 0) {
        return ret;
    }

    if(BSP_IIC_Probe(BSP_LSM303DLHC_MAG_ADDR_7BIT) != 0) {
        return -3;
    }

    // 加速度计：100Hz，XYZ 轴开启，高分辨率，块更新，量程 +/-2g。
    if(BSP_IIC_WriteReg(lsm303dlhc_accel_addr,
                        LSM303DLHC_REG_CTRL_REG1_A,
                        LSM303DLHC_CTRL_REG1_A_100HZ_XYZ) != 0) {
        return -4;
    }

    if(BSP_IIC_WriteReg(lsm303dlhc_accel_addr,
                        LSM303DLHC_REG_CTRL_REG4_A,
                        LSM303DLHC_CTRL_REG4_A_2G_HR_BDU) != 0) {
        return -5;
    }

    // 磁力计：15Hz，+/-1.3 gauss，连续转换模式。
    if(BSP_IIC_WriteReg(BSP_LSM303DLHC_MAG_ADDR_7BIT,
                        LSM303DLHC_REG_CRA_REG_M,
                        LSM303DLHC_CRA_REG_M_15HZ) != 0) {
        return -6;
    }

    if(BSP_IIC_WriteReg(BSP_LSM303DLHC_MAG_ADDR_7BIT,
                        LSM303DLHC_REG_CRB_REG_M,
                        LSM303DLHC_CRB_REG_M_1_3GAUSS) != 0) {
        return -7;
    }

    if(BSP_IIC_WriteReg(BSP_LSM303DLHC_MAG_ADDR_7BIT,
                        LSM303DLHC_REG_MR_REG_M,
                        LSM303DLHC_MR_REG_M_CONTINUOUS) != 0) {
        return -8;
    }

    lsm303dlhc_initialized = 1U;
    return 0;
}

/**
 * @brief 探测 LSM303DLHC 加速度计和磁力计是否响应。
 */
int BSP_LSM303DLHC_Probe(void)
{
    BSP_IIC_Init();

    if(BSP_LSM303DLHC_DetectAccelAddress() != 0) {
        return -1;
    }

    if(BSP_IIC_Probe(BSP_LSM303DLHC_MAG_ADDR_7BIT) != 0) {
        return -2;
    }

    return 0;
}

/**
 * @brief 读取加速度计三轴数据。
 */
int BSP_LSM303DLHC_ReadAccel(BSP_LSM303DLHC_Vector_t * raw,
                             BSP_LSM303DLHC_Vector_t * mg)
{
    uint8_t buf[6];

    if(raw == NULL) {
        return -1;
    }

    if(lsm303dlhc_initialized == 0U) {
        int ret = BSP_LSM303DLHC_Init();
        if(ret != 0) {
            return ret;
        }
    }

    if(BSP_IIC_ReadRegs(lsm303dlhc_accel_addr,
                        (uint8_t)(LSM303DLHC_REG_OUT_X_L_A | LSM303DLHC_ACCEL_AUTO_INC),
                        buf,
                        sizeof(buf)) != 0) {
        return -2;
    }

    // LSM303DLHC 加速度计数据为小端左对齐；高分辨率 12 位模式右移 4 位得到计数。
    raw->x = (int16_t)(BSP_LSM303DLHC_CombineLe(&buf[0]) >> 4);
    raw->y = (int16_t)(BSP_LSM303DLHC_CombineLe(&buf[2]) >> 4);
    raw->z = (int16_t)(BSP_LSM303DLHC_CombineLe(&buf[4]) >> 4);

    if(mg != NULL) {
        // 默认 +/-2g 高分辨率配置下灵敏度约 1 mg/LSB。
        mg->x = raw->x;
        mg->y = raw->y;
        mg->z = raw->z;
    }

    return 0;
}

/**
 * @brief 读取磁力计三轴数据。
 */
int BSP_LSM303DLHC_ReadMag(BSP_LSM303DLHC_Vector_t * raw,
                           BSP_LSM303DLHC_Vector_t * mgauss)
{
    uint8_t buf[6];

    if(raw == NULL) {
        return -1;
    }

    if(lsm303dlhc_initialized == 0U) {
        int ret = BSP_LSM303DLHC_Init();
        if(ret != 0) {
            return ret;
        }
    }

    if(BSP_IIC_ReadRegs(BSP_LSM303DLHC_MAG_ADDR_7BIT,
                        LSM303DLHC_REG_OUT_X_H_M,
                        buf,
                        sizeof(buf)) != 0) {
        return -2;
    }

    // 磁力计寄存器顺序是 X、Z、Y，且每轴为大端格式。
    raw->x = BSP_LSM303DLHC_CombineBe(buf[0], buf[1]);
    raw->z = BSP_LSM303DLHC_CombineBe(buf[2], buf[3]);
    raw->y = BSP_LSM303DLHC_CombineBe(buf[4], buf[5]);

    if(mgauss != NULL) {
        mgauss->x = BSP_LSM303DLHC_ScaleMag(raw->x, LSM303DLHC_MAG_XY_SENS_LSB_PER_GAUSS);
        mgauss->y = BSP_LSM303DLHC_ScaleMag(raw->y, LSM303DLHC_MAG_XY_SENS_LSB_PER_GAUSS);
        mgauss->z = BSP_LSM303DLHC_ScaleMag(raw->z, LSM303DLHC_MAG_Z_SENS_LSB_PER_GAUSS);
    }

    return 0;
}

/**
 * @brief 连续读取加速度计和磁力计数据。
 */
int BSP_LSM303DLHC_Read(BSP_LSM303DLHC_Data_t * data)
{
    int ret;

    if(data == NULL) {
        return -1;
    }

    ret = BSP_LSM303DLHC_ReadAccel(&data->accel_raw, &data->accel_mg);
    if(ret != 0) {
        return ret;
    }

    ret = BSP_LSM303DLHC_ReadMag(&data->mag_raw, &data->mag_mgauss);
    if(ret != 0) {
        return ret;
    }

    return 0;
}

/**
 * @brief 自动探测加速度计 SA0 对应的 7 位地址并校验 WHO_AM_I。
 */
static int BSP_LSM303DLHC_DetectAccelAddress(void)
{
    uint8_t id;

    if((BSP_LSM303DLHC_ReadAccelWhoAmI(BSP_LSM303DLHC_ACCEL_ADDR_HIGH_7BIT, &id) == 0) &&
       (id == LSM303DLHC_WHO_AM_I_A_VALUE)) {
        lsm303dlhc_accel_addr = BSP_LSM303DLHC_ACCEL_ADDR_HIGH_7BIT;
        return 0;
    }

    if((BSP_LSM303DLHC_ReadAccelWhoAmI(BSP_LSM303DLHC_ACCEL_ADDR_LOW_7BIT, &id) == 0) &&
       (id == LSM303DLHC_WHO_AM_I_A_VALUE)) {
        lsm303dlhc_accel_addr = BSP_LSM303DLHC_ACCEL_ADDR_LOW_7BIT;
        return 0;
    }

    return -1;
}

/**
 * @brief 读取加速度计 WHO_AM_I 寄存器。
 */
static int BSP_LSM303DLHC_ReadAccelWhoAmI(uint8_t addr, uint8_t * value)
{
    if(value == NULL) {
        return -1;
    }

    return BSP_IIC_ReadRegs(addr, LSM303DLHC_REG_WHO_AM_I_A, value, 1U);
}

/**
 * @brief 合成小端 16 位有符号数。
 */
static int16_t BSP_LSM303DLHC_CombineLe(const uint8_t * data)
{
    return (int16_t)((uint16_t)data[0] | ((uint16_t)data[1] << 8));
}

/**
 * @brief 合成大端 16 位有符号数。
 */
static int16_t BSP_LSM303DLHC_CombineBe(uint8_t high, uint8_t low)
{
    return (int16_t)(((uint16_t)high << 8) | (uint16_t)low);
}

/**
 * @brief 将磁力计原始计数换算为毫高斯。
 */
static int16_t BSP_LSM303DLHC_ScaleMag(int16_t raw, int32_t sensitivity)
{
    int32_t scaled = ((int32_t)raw * 1000L) / sensitivity;

    if(scaled > 32767L) {
        return 32767;
    }

    if(scaled < -32768L) {
        return (int16_t)-32768;
    }

    return (int16_t)scaled;
}
