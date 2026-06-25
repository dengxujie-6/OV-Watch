#ifndef BSP_IIC_H
#define BSP_IIC_H

#include <stdint.h>

#include "stm32f4xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化软件 IIC 总线。
 *
 * 本总线固定使用 PB13 作为 SDA、PB14 作为 SCL。函数只配置 GPIO
 * 和软件 IIC 空闲电平，不假设任何具体器件型号或器件地址。
 */
void BSP_IIC_Init(void);

/**
 * @brief 反初始化软件 IIC GPIO，并清除总线已初始化状态。
 *
 * 该接口用于系统进入低功耗前释放 PB13/PB14；恢复后再次调用 BSP_IIC_Init()
 * 即可重新建立软件 IIC 总线。
 */
void BSP_IIC_DeInit(void);

/**
 * @brief 探测 7 位 I2C 设备地址是否有 ACK。
 *
 * @param dev_addr_7bit I2C 从机 7 位地址，不包含读写位。
 * @return 0 表示收到 ACK，负数表示无 ACK 或参数非法。
 */
int BSP_IIC_Probe(uint8_t dev_addr_7bit);

/**
 * @brief 向 7 位地址设备连续写入原始字节。
 *
 * @param dev_addr_7bit I2C 从机 7 位地址，不包含读写位。
 * @param data 待发送数据缓冲区，不能为空。
 * @param len 待发送字节数，必须大于 0。
 * @return 0 表示成功，负数表示参数非法或总线 NACK。
 */
int BSP_IIC_Write(uint8_t dev_addr_7bit, const uint8_t * data, uint16_t len);

/**
 * @brief 从 7 位地址设备连续读取原始字节。
 *
 * @param dev_addr_7bit I2C 从机 7 位地址，不包含读写位。
 * @param data 接收数据缓冲区，不能为空。
 * @param len 待读取字节数，必须大于 0。
 * @return 0 表示成功，负数表示参数非法或总线 NACK。
 */
int BSP_IIC_Read(uint8_t dev_addr_7bit, uint8_t * data, uint16_t len);

/**
 * @brief 先写入一段命令字节，再重复起始读取数据。
 *
 * 多数传感器读取寄存器时，需要先写寄存器地址或命令，再使用 repeated START
 * 切换到读方向。本接口只负责总线时序，不假设具体传感器型号或寄存器含义。
 *
 * @param dev_addr_7bit I2C 从机 7 位地址，不包含读写位。
 * @param tx_data 写阶段缓冲区，不能为空。
 * @param tx_len 写阶段字节数，必须大于 0。
 * @param rx_data 读阶段接收缓冲区，不能为空。
 * @param rx_len 读阶段字节数，必须大于 0。
 * @return 0 表示成功，负数表示参数非法或总线 NACK。
 */
int BSP_IIC_WriteRead(uint8_t dev_addr_7bit,
                      const uint8_t * tx_data,
                      uint16_t tx_len,
                      uint8_t * rx_data,
                      uint16_t rx_len);

/**
 * @brief 写入 8 位寄存器地址的单字节寄存器。
 *
 * @param dev_addr_7bit I2C 从机 7 位地址，不包含读写位。
 * @param reg 8 位寄存器地址。
 * @param value 写入值。
 * @return 0 表示成功，负数表示总线 NACK。
 */
int BSP_IIC_WriteReg(uint8_t dev_addr_7bit, uint8_t reg, uint8_t value);

/**
 * @brief 从 8 位寄存器地址连续读取寄存器。
 *
 * @param dev_addr_7bit I2C 从机 7 位地址，不包含读写位。
 * @param reg 起始 8 位寄存器地址。
 * @param data 接收数据缓冲区，不能为空。
 * @param len 待读取字节数，必须大于 0。
 * @return 0 表示成功，负数表示参数非法或总线 NACK。
 */
int BSP_IIC_ReadRegs(uint8_t dev_addr_7bit, uint8_t reg, uint8_t * data, uint16_t len);

/**
 * @brief 获取最近一次软件 IIC 事务对应的 HAL 状态码。
 *
 * 该接口用于上层诊断统计，不改变现有返回值约定。
 *
 * @return 最近一次事务映射得到的 HAL_StatusTypeDef。
 */
HAL_StatusTypeDef BSP_IIC_GetLastHalStatus(void);

/**
 * @brief 获取最近一次软件 IIC 事务对应的 HAL 错误码。
 *
 * 当前软件 IIC 不使用 HAL I2C 外设，该值主要用于区分是否发生 ACK 超时。
 *
 * @return 最近一次事务的错误码，成功时返回 0。
 */
uint32_t BSP_IIC_GetLastHalError(void);

#ifdef __cplusplus
}
#endif

#endif /* BSP_IIC_H */
