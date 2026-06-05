#ifndef BSP_IIC_H
#define BSP_IIC_H

#include <stdint.h>

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

#ifdef __cplusplus
}
#endif

#endif /* BSP_IIC_H */
