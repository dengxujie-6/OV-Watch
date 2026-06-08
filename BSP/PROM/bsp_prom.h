#ifndef BSP_PROM_H
#define BSP_PROM_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BSP_PROM_SIZE_BYTES       256U
#define BSP_PROM_PAGE_SIZE_BYTES  16U

#ifndef BSP_PROM_I2C_ADDR_7BIT
#define BSP_PROM_I2C_ADDR_7BIT    0x50U
#endif

/**
 * @brief 初始化 BL24C02F PROM 及其专用软件 I2C 总线。
 */
void BSP_PROM_Init(void);

/**
 * @brief 探测 BL24C02F 是否响应当前 7 位 I2C 地址。
 *
 * @return 0 表示收到 ACK，负数表示无响应。
 */
int BSP_PROM_Probe(void);

/**
 * @brief 从 BL24C02F 读取连续字节。
 *
 * @param addr 起始 EEPROM 字节地址，范围 0~255。
 * @param data 接收缓冲区，不能为 NULL。
 * @param len 读取长度，必须大于 0，且不能越过 256 字节容量。
 * @return 0 表示成功，负数表示参数非法或总线错误。
 */
int BSP_PROM_Read(uint8_t addr, uint8_t * data, uint16_t len);

/**
 * @brief 向 BL24C02F 写入连续字节。
 *
 * 写入会自动按 16 字节页边界拆分，避免页写回卷覆盖同页前面的数据。
 *
 * @param addr 起始 EEPROM 字节地址，范围 0~255。
 * @param data 待写入缓冲区，不能为 NULL。
 * @param len 写入长度，必须大于 0，且不能越过 256 字节容量。
 * @return 0 表示成功，负数表示参数非法、总线错误或写周期超时。
 */
int BSP_PROM_Write(uint8_t addr, const uint8_t * data, uint16_t len);

/**
 * @brief 读取 BL24C02F 单字节。
 *
 * @param addr EEPROM 字节地址，范围 0~255。
 * @param value 接收字节指针，不能为 NULL。
 * @return 0 表示成功，负数表示参数非法或总线错误。
 */
int BSP_PROM_ReadByte(uint8_t addr, uint8_t * value);

/**
 * @brief 写入 BL24C02F 单字节。
 *
 * @param addr EEPROM 字节地址，范围 0~255。
 * @param value 待写入字节。
 * @return 0 表示成功，负数表示总线错误或写周期超时。
 */
int BSP_PROM_WriteByte(uint8_t addr, uint8_t value);

#ifdef __cplusplus
}
#endif

#endif /* BSP_PROM_H */
