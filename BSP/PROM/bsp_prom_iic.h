#ifndef BSP_PROM_IIC_H
#define BSP_PROM_IIC_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化 PROM 专用软件 I2C 总线。
 *
 * 本总线固定使用 PA11 作为 SDA、PA12 作为 SCL，GPIO 配置为开漏并启用上拉。
 */
void BSP_PROM_IIC_Init(void);

/**
 * @brief 探测 7 位 I2C 设备地址是否响应 ACK。
 *
 * @param dev_addr_7bit I2C 从机 7 位地址，不包含读写位。
 * @return 0 表示收到 ACK，负数表示参数非法或无 ACK。
 */
int BSP_PROM_IIC_Probe(uint8_t dev_addr_7bit);

/**
 * @brief 向 7 位地址设备连续写入原始字节。
 *
 * @param dev_addr_7bit I2C 从机 7 位地址，不包含读写位。
 * @param data 待发送数据缓冲区，不能为 NULL。
 * @param len 待发送字节数，必须大于 0。
 * @return 0 表示成功，负数表示参数非法或总线 NACK。
 */
int BSP_PROM_IIC_Write(uint8_t dev_addr_7bit, const uint8_t * data, uint16_t len);

/**
 * @brief 从 7 位地址设备连续读取原始字节。
 *
 * @param dev_addr_7bit I2C 从机 7 位地址，不包含读写位。
 * @param data 接收数据缓冲区，不能为 NULL。
 * @param len 待读取字节数，必须大于 0。
 * @return 0 表示成功，负数表示参数非法或总线 NACK。
 */
int BSP_PROM_IIC_Read(uint8_t dev_addr_7bit, uint8_t * data, uint16_t len);

/**
 * @brief 先写入一段命令字节，再重复起始读取数据。
 *
 * EEPROM 随机读需要先写入内部字地址，然后使用 repeated START 切到读方向。
 *
 * @param dev_addr_7bit I2C 从机 7 位地址，不包含读写位。
 * @param tx_data 写阶段缓冲区，不能为 NULL。
 * @param tx_len 写阶段字节数，必须大于 0。
 * @param rx_data 读阶段接收缓冲区，不能为 NULL。
 * @param rx_len 读阶段字节数，必须大于 0。
 * @return 0 表示成功，负数表示参数非法或总线 NACK。
 */
int BSP_PROM_IIC_WriteRead(uint8_t dev_addr_7bit,
                           const uint8_t * tx_data,
                           uint16_t tx_len,
                           uint8_t * rx_data,
                           uint16_t rx_len);

#ifdef __cplusplus
}
#endif

#endif /* BSP_PROM_IIC_H */
