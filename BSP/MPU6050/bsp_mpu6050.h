#ifndef BSP_MPU6050_H
#define BSP_MPU6050_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BSP_MPU6050_ADDR_LOW_7BIT
#define BSP_MPU6050_ADDR_LOW_7BIT   0x68U
#endif

#ifndef BSP_MPU6050_ADDR_HIGH_7BIT
#define BSP_MPU6050_ADDR_HIGH_7BIT  0x69U
#endif

/**
 * @brief MPU6050 三轴有符号向量。
 */
typedef struct {
    int16_t x;
    int16_t y;
    int16_t z;
} BSP_MPU6050_Vector_t;

/**
 * @brief MPU6050 单次采样数据。
 *
 * accel_raw 和 gyro_raw 是寄存器原始计数；默认初始化配置为加速度 +/-2g、
 * 陀螺仪 +/-250dps，因此 accel_mg 单位为 mg，gyro_x10_dps 单位为 0.1dps。
 * temperature_x10_c 使用 0.1 摄氏度为单位。
 */
typedef struct {
    BSP_MPU6050_Vector_t accel_raw;
    BSP_MPU6050_Vector_t accel_mg;
    BSP_MPU6050_Vector_t gyro_raw;
    BSP_MPU6050_Vector_t gyro_x10_dps;
    int16_t temperature_raw;
    int16_t temperature_x10_c;
} BSP_MPU6050_Data_t;

/**
 * @brief 初始化 MPU6050 并配置默认量程。
 *
 * 驱动会自动探测 AD0 对应的 0x68/0x69 两个 7 位地址，并通过 WHO_AM_I 校验芯片。
 *
 * @return 0 表示成功，负数表示无 ACK、ID 不匹配或寄存器配置失败。
 */
int BSP_MPU6050_Init(void);

/**
 * @brief 探测 MPU6050 是否响应并校验 WHO_AM_I。
 *
 * @return 0 表示找到有效芯片，负数表示 0x68/0x69 均不可用。
 */
int BSP_MPU6050_Probe(void);

/**
 * @brief 读取 MPU6050 加速度、温度和陀螺仪数据。
 *
 * @param data 接收采样结果的指针，不能为 NULL，调用者拥有该对象。
 * @return 0 表示成功，负数表示参数非法、初始化失败或总线读取失败。
 */
int BSP_MPU6050_Read(BSP_MPU6050_Data_t * data);

/**
 * @brief 配置 MPU6050 在普通运行电源状态下输出运动唤醒中断。
 *
 * 该接口用于 MCU 进入 Sleep 前保留 MPU6050 工作，并在检测到明显运动时通过 INT
 * 引脚拉高唤醒 EXTI。阈值和持续时间采用项目内保守默认值，后续可按实机效果再调。
 *
 * @return 0 表示成功，负数表示初始化或寄存器配置失败。
 */
int BSP_MPU6050_EnableWakeOnMotion(void);

/**
 * @brief 清除运动唤醒中断配置并恢复常规采样模式。
 *
 * @return 0 表示成功，负数表示恢复失败。
 */
int BSP_MPU6050_DisableWakeOnMotion(void);

/**
 * @brief 打开 MPU6050 Data Ready 中断输出。
 *
 * @return 0 表示成功，负数表示寄存器配置失败。
 */
int BSP_MPU6050_EnableDataReadyInterrupt(void);

/**
 * @brief 关闭 MPU6050 Data Ready 中断输出。
 *
 * @return 0 表示成功，负数表示寄存器配置失败。
 */
int BSP_MPU6050_DisableDataReadyInterrupt(void);

#ifdef __cplusplus
}
#endif

#endif /* BSP_MPU6050_H */
