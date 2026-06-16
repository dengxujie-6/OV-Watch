#ifndef BSP_LSM303DLHC_H
#define BSP_LSM303DLHC_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BSP_LSM303DLHC_ACCEL_ADDR_HIGH_7BIT
#define BSP_LSM303DLHC_ACCEL_ADDR_HIGH_7BIT  0x19U
#endif

#ifndef BSP_LSM303DLHC_ACCEL_ADDR_LOW_7BIT
#define BSP_LSM303DLHC_ACCEL_ADDR_LOW_7BIT   0x18U
#endif

#ifndef BSP_LSM303DLHC_MAG_ADDR_7BIT
#define BSP_LSM303DLHC_MAG_ADDR_7BIT         0x1EU
#endif

/**
 * @brief 三轴有符号向量。
 */
typedef struct {
    int16_t x;
    int16_t y;
    int16_t z;
} BSP_LSM303DLHC_Vector_t;

/**
 * @brief LSM303DLHC 单次采样数据。
 *
 * accel_raw 是右对齐后的加速度计原始计数；默认 +/-2g 高分辨率配置下，
 * accel_mg 约等于 mg 单位值。mag_raw 是磁力计原始计数，mag_mgauss 为毫高斯。
 */
typedef struct {
    BSP_LSM303DLHC_Vector_t accel_raw;
    BSP_LSM303DLHC_Vector_t accel_mg;
    BSP_LSM303DLHC_Vector_t mag_raw;
    BSP_LSM303DLHC_Vector_t mag_mgauss;
} BSP_LSM303DLHC_Data_t;

/**
 * @brief 初始化 LSM303DLHC 加速度计和磁力计。
 *
 * @return 0 表示成功，负数表示总线无响应、芯片 ID 不匹配或寄存器配置失败。
 */
int BSP_LSM303DLHC_Init(void);

/**
 * @brief 探测 LSM303DLHC 加速度计和磁力计是否响应。
 *
 * @return 0 表示两部分均响应，负数表示加速度计或磁力计无响应。
 */
int BSP_LSM303DLHC_Probe(void);

/**
 * @brief 读取加速度计三轴数据。
 *
 * @param raw 接收原始计数的指针，不能为 NULL，调用者拥有该对象。
 * @param mg 接收 mg 换算值的指针，可以为 NULL。
 * @return 0 表示成功，负数表示参数非法、未初始化或总线读取失败。
 */
int BSP_LSM303DLHC_ReadAccel(BSP_LSM303DLHC_Vector_t * raw,
                             BSP_LSM303DLHC_Vector_t * mg);

/**
 * @brief 读取磁力计三轴数据。
 *
 * @param raw 接收原始计数的指针，不能为 NULL，调用者拥有该对象。
 * @param mgauss 接收毫高斯换算值的指针，可以为 NULL。
 * @return 0 表示成功，负数表示参数非法、未初始化或总线读取失败。
 */
int BSP_LSM303DLHC_ReadMag(BSP_LSM303DLHC_Vector_t * raw,
                           BSP_LSM303DLHC_Vector_t * mgauss);

/**
 * @brief 连续读取加速度计和磁力计数据。
 *
 * @param data 接收完整采样数据的指针，不能为 NULL，调用者拥有该对象。
 * @return 0 表示成功，负数表示任一子设备读取失败。
 */
int BSP_LSM303DLHC_Read(BSP_LSM303DLHC_Data_t * data);

#ifdef __cplusplus
}
#endif

#endif /* BSP_LSM303DLHC_H */
