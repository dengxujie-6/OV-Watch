/**
 * @file bsp_prom.c
 * @brief BL24C02F 2Kbit I2C EEPROM 驱动。
 */

#include "bsp_prom.h"

#include <stddef.h>

#include "bsp_prom_iic.h"
#include "stm32f4xx_hal.h"

#define BSP_PROM_WRITE_CYCLE_TIMEOUT_MS  10U
#define BSP_PROM_PAGE_BUFFER_BYTES       (BSP_PROM_PAGE_SIZE_BYTES + 1U)

static uint8_t prom_initialized;

static int BSP_PROM_CheckRange(uint8_t addr, uint16_t len);
static int BSP_PROM_WritePage(uint8_t addr, const uint8_t * data, uint8_t len);
static int BSP_PROM_WaitReady(void);

/**
 * @brief 初始化 BL24C02F PROM 及其专用软件 I2C 总线。
 */
void BSP_PROM_Init(void)
{
    BSP_PROM_IIC_Init();
    prom_initialized = 1U;
}

/**
 * @brief 探测 BL24C02F 是否响应当前 7 位 I2C 地址。
 */
int BSP_PROM_Probe(void)
{
    if(prom_initialized == 0U) {
        BSP_PROM_Init();
    }

    return BSP_PROM_IIC_Probe(BSP_PROM_I2C_ADDR_7BIT);
}

/**
 * @brief 从 BL24C02F 读取连续字节。
 */
int BSP_PROM_Read(uint8_t addr, uint8_t * data, uint16_t len)
{
    if((data == NULL) || (BSP_PROM_CheckRange(addr, len) != 0)) {
        return -1;
    }

    if(prom_initialized == 0U) {
        BSP_PROM_Init();
    }

    return BSP_PROM_IIC_WriteRead(BSP_PROM_I2C_ADDR_7BIT, &addr, 1U, data, len);
}

/**
 * @brief 向 BL24C02F 写入连续字节。
 */
int BSP_PROM_Write(uint8_t addr, const uint8_t * data, uint16_t len)
{
    uint8_t current_addr = addr;
    const uint8_t * current_data = data;
    uint16_t remaining = len;

    if((data == NULL) || (BSP_PROM_CheckRange(addr, len) != 0)) {
        return -1;
    }

    if(prom_initialized == 0U) {
        BSP_PROM_Init();
    }

    while(remaining > 0U) {
        uint8_t page_offset = (uint8_t)(current_addr % BSP_PROM_PAGE_SIZE_BYTES);
        uint8_t page_space = (uint8_t)(BSP_PROM_PAGE_SIZE_BYTES - page_offset);
        uint8_t chunk = (remaining > page_space) ? page_space : (uint8_t)remaining;
        int ret;

        ret = BSP_PROM_WritePage(current_addr, current_data, chunk);
        if(ret != 0) {
            return ret;
        }

        ret = BSP_PROM_WaitReady();
        if(ret != 0) {
            return ret;
        }

        current_addr = (uint8_t)(current_addr + chunk);
        current_data += chunk;
        remaining = (uint16_t)(remaining - chunk);
    }

    return 0;
}

/**
 * @brief 读取 BL24C02F 单字节。
 */
int BSP_PROM_ReadByte(uint8_t addr, uint8_t * value)
{
    return BSP_PROM_Read(addr, value, 1U);
}

/**
 * @brief 写入 BL24C02F 单字节。
 */
int BSP_PROM_WriteByte(uint8_t addr, uint8_t value)
{
    return BSP_PROM_Write(addr, &value, 1U);
}

/**
 * @brief 检查访问范围是否落在 256 字节 EEPROM 内。
 */
static int BSP_PROM_CheckRange(uint8_t addr, uint16_t len)
{
    uint16_t end;

    if(len == 0U) {
        return -1;
    }

    end = (uint16_t)addr + len;
    if(end > BSP_PROM_SIZE_BYTES) {
        return -2;
    }

    return 0;
}

/**
 * @brief 在单个 BL24C02F 页内执行一次页写。
 */
static int BSP_PROM_WritePage(uint8_t addr, const uint8_t * data, uint8_t len)
{
    uint8_t buffer[BSP_PROM_PAGE_BUFFER_BYTES];
    uint8_t i;

    if((data == NULL) || (len == 0U) || (len > BSP_PROM_PAGE_SIZE_BYTES)) {
        return -1;
    }

    buffer[0] = addr;
    for(i = 0U; i < len; i++) {
        buffer[(uint8_t)(i + 1U)] = data[i];
    }

    if(BSP_PROM_IIC_Write(BSP_PROM_I2C_ADDR_7BIT, buffer, (uint16_t)(len + 1U)) != 0) {
        return -2;
    }

    return 0;
}

/**
 * @brief 等待 EEPROM 内部写周期完成。
 *
 * BL24C02F 页写是自定时写周期。写入后器件会暂时 NACK，直到内部写入完成。
 */
static int BSP_PROM_WaitReady(void)
{
    uint32_t start = HAL_GetTick();

    do {
        if(BSP_PROM_IIC_Probe(BSP_PROM_I2C_ADDR_7BIT) == 0) {
            return 0;
        }
    } while((HAL_GetTick() - start) <= BSP_PROM_WRITE_CYCLE_TIMEOUT_MS);

    return -1;
}
