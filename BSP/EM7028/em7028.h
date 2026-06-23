#ifndef EM7028_H
#define EM7028_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief EM7028 驱动状态码。
 */
typedef enum
{
    EM7028_OK = 0,
    EM7028_ERR_PARAM,
    EM7028_ERR_I2C,
    EM7028_ERR_ID,
    EM7028_ERR_NOT_INITIALIZED,
    EM7028_ERR_NOT_RUNNING,
    EM7028_ERR_ALREADY_RUNNING
} EM7028_Status_t;

/**
 * @brief EM7028 HRS1 连续模式配置结构体。
 */
typedef struct
{
    uint8_t hrs_gain;
    uint8_t hrs_range;
    uint8_t hrs_freq;
    uint8_t hrs_resolution;
} EM7028_Hrs1Config_t;

/**
 * @brief EM7028 设备状态句柄。
 */
typedef struct
{
    uint8_t product_id;
    uint8_t initialized;
    uint8_t hrs1_running;
    uint16_t latest_raw_ppg;
} EM7028_Handle_t;

/**
 * @brief 获取 EM7028 HRS1 默认配置。
 *
 * @param config HRS1 配置输出指针。
 */
void EM7028_GetDefaultHrs1Config(EM7028_Hrs1Config_t * config);

/**
 * @brief 读取 EM7028 单个寄存器。
 *
 * @param reg 寄存器地址。
 * @param value 读取结果输出指针。
 * @return EM7028_Status_t 操作结果。
 */
EM7028_Status_t EM7028_ReadReg(uint8_t reg, uint8_t * value);

/**
 * @brief 写入 EM7028 单个寄存器。
 *
 * @param reg 寄存器地址。
 * @param value 要写入的数据。
 * @return EM7028_Status_t 操作结果。
 */
EM7028_Status_t EM7028_WriteReg(uint8_t reg, uint8_t value);

/**
 * @brief 连续读取 EM7028 寄存器数据。
 *
 * @param start_reg 起始寄存器地址。
 * @param buffer 数据缓冲区。
 * @param length 读取字节数。
 * @return EM7028_Status_t 操作结果。
 */
EM7028_Status_t EM7028_ReadRegs(uint8_t start_reg, uint8_t * buffer, uint8_t length);

/**
 * @brief 读取并校验 EM7028 产品 ID。
 *
 * @param handle EM7028 设备句柄。
 * @return EM7028_Status_t 操作结果。
 */
EM7028_Status_t EM7028_CheckDeviceId(EM7028_Handle_t * handle);

/**
 * @brief 初始化 EM7028 BSP 驱动。
 *
 * @param handle EM7028 设备句柄。
 * @param config HRS1 默认配置。
 * @return EM7028_Status_t 操作结果。
 */
EM7028_Status_t EM7028_Init(EM7028_Handle_t * handle, const EM7028_Hrs1Config_t * config);

/**
 * @brief 启动 EM7028 HRS1 连续采样模式。
 *
 * @param handle EM7028 设备句柄。
 * @return EM7028_Status_t 操作结果。
 */
EM7028_Status_t EM7028_StartHrs1(EM7028_Handle_t * handle);

/**
 * @brief 停止 EM7028 HRS1 连续采样模式。
 *
 * @param handle EM7028 设备句柄。
 * @return EM7028_Status_t 操作结果。
 */
EM7028_Status_t EM7028_StopHrs1(EM7028_Handle_t * handle);

/**
 * @brief 读取一次 EM7028 的 16 位 HRS1 原始 PPG 数据。
 *
 * @param handle EM7028 设备句柄。
 * @param raw_ppg 原始 PPG 数据输出指针。
 * @return EM7028_Status_t 操作结果。
 */
EM7028_Status_t EM7028_ReadHrs1Raw(EM7028_Handle_t * handle, uint16_t * raw_ppg);

#ifdef __cplusplus
}
#endif

#endif /* EM7028_H */
