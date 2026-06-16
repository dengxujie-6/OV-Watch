/**
 * @file bsp_aht21.c
 * @brief AHT21 温湿度传感器驱动。
 */

#include "bsp_aht21.h"

#include <stddef.h>

#include "bsp_iic.h"
#include "stm32f4xx_hal.h"

#define BSP_AHT21_CMD_INIT             0xBEU
#define BSP_AHT21_CMD_TRIGGER_MEASURE  0xACU
#define BSP_AHT21_CMD_SOFT_RESET       0xBAU

#define BSP_AHT21_INIT_PARAM_0         0x08U
#define BSP_AHT21_INIT_PARAM_1         0x00U
#define BSP_AHT21_MEASURE_PARAM_0      0x33U
#define BSP_AHT21_MEASURE_PARAM_1      0x00U

#define BSP_AHT21_STATUS_BUSY_MASK     0x80U
#define BSP_AHT21_STATUS_CAL_MASK      0x08U
#define BSP_AHT21_POWER_ON_DELAY_MS    40U
#define BSP_AHT21_INIT_DELAY_MS        10U
#define BSP_AHT21_MEASURE_DELAY_MS     80U
#define BSP_AHT21_RESET_DELAY_MS       20U
#define BSP_AHT21_RAW_FULL_SCALE       1048576UL

static uint8_t aht21_initialized;

static int BSP_AHT21_ReadStatus(uint8_t * status);
static int BSP_AHT21_SendInitCommand(void);
static uint8_t BSP_AHT21_CalcCrc8(const uint8_t * data, uint8_t len);

/**
 * @brief 初始化 AHT21 所在的软件 I2C 总线并确认芯片校准状态。
 */
int BSP_AHT21_Init(void)
{
    uint8_t status;

    BSP_IIC_Init();
    HAL_Delay(BSP_AHT21_POWER_ON_DELAY_MS);

    if(BSP_IIC_Probe(BSP_AHT21_I2C_ADDR_7BIT) != 0) {
        return -1;
    }

    if(BSP_AHT21_ReadStatus(&status) != 0) {
        return -2;
    }

    if((status & BSP_AHT21_STATUS_CAL_MASK) == 0U) {
        if(BSP_AHT21_SendInitCommand() != 0) {
            return -3;
        }

        if(BSP_AHT21_ReadStatus(&status) != 0) {
            return -4;
        }

        if((status & BSP_AHT21_STATUS_CAL_MASK) == 0U) {
            return -5;
        }
    }

    aht21_initialized = 1U;
    return 0;
}

/**
 * @brief 探测 AHT21 默认 7 位 I2C 地址是否响应 ACK。
 */
int BSP_AHT21_Probe(void)
{
    if(aht21_initialized == 0U) {
        BSP_IIC_Init();
    }

    return BSP_IIC_Probe(BSP_AHT21_I2C_ADDR_7BIT);
}

/**
 * @brief 触发一次 AHT21 温湿度测量并读取转换结果。
 */
int BSP_AHT21_Read(BSP_AHT21_Data_t * data)
{
    uint8_t cmd[3];
    uint8_t raw[7];
    uint32_t humidity_raw;
    uint32_t temperature_raw;
    uint32_t humidity_x10;
    int32_t temperature_x10;

    if(data == NULL) {
        return -1;
    }

    if(aht21_initialized == 0U) {
        int init_ret = BSP_AHT21_Init();
        if(init_ret != 0) {
            return init_ret;
        }
    }

    cmd[0] = BSP_AHT21_CMD_TRIGGER_MEASURE;
    cmd[1] = BSP_AHT21_MEASURE_PARAM_0;
    cmd[2] = BSP_AHT21_MEASURE_PARAM_1;
    if(BSP_IIC_Write(BSP_AHT21_I2C_ADDR_7BIT, cmd, sizeof(cmd)) != 0) {
        return -2;
    }

    // AHT21 完成一次温湿度转换需要等待，期间不占用总线轮询。
    HAL_Delay(BSP_AHT21_MEASURE_DELAY_MS);

    if(BSP_IIC_Read(BSP_AHT21_I2C_ADDR_7BIT, raw, sizeof(raw)) != 0) {
        return -3;
    }

    if((raw[0] & BSP_AHT21_STATUS_BUSY_MASK) != 0U) {
        return -4;
    }

    if(BSP_AHT21_CalcCrc8(raw, 6U) != raw[6]) {
        return -5;
    }

    humidity_raw = ((uint32_t)raw[1] << 12) |
                   ((uint32_t)raw[2] << 4) |
                   ((uint32_t)raw[3] >> 4);
    temperature_raw = (((uint32_t)raw[3] & 0x0FUL) << 16) |
                      ((uint32_t)raw[4] << 8) |
                      (uint32_t)raw[5];

    humidity_x10 = (humidity_raw * 1000UL) / BSP_AHT21_RAW_FULL_SCALE;
    if(humidity_x10 > 1000UL) {
        humidity_x10 = 1000UL;
    }

    temperature_x10 = (int32_t)((temperature_raw * 2000UL) / BSP_AHT21_RAW_FULL_SCALE) - 500;

    data->humidity_x10_percent = (uint16_t)humidity_x10;
    data->temperature_x10_c = (int16_t)temperature_x10;

    return 0;
}

/**
 * @brief 向 AHT21 发送软件复位命令。
 */
int BSP_AHT21_SoftReset(void)
{
    uint8_t cmd = BSP_AHT21_CMD_SOFT_RESET;

    if(aht21_initialized == 0U) {
        BSP_IIC_Init();
    }

    if(BSP_IIC_Write(BSP_AHT21_I2C_ADDR_7BIT, &cmd, 1U) != 0) {
        return -1;
    }

    aht21_initialized = 0U;
    HAL_Delay(BSP_AHT21_RESET_DELAY_MS);

    return 0;
}

/**
 * @brief 读取 AHT21 一个状态字节。
 */
static int BSP_AHT21_ReadStatus(uint8_t * status)
{
    if(status == NULL) {
        return -1;
    }

    return BSP_IIC_Read(BSP_AHT21_I2C_ADDR_7BIT, status, 1U);
}

/**
 * @brief 发送 AHT21 初始化校准命令。
 */
static int BSP_AHT21_SendInitCommand(void)
{
    uint8_t cmd[3];

    cmd[0] = BSP_AHT21_CMD_INIT;
    cmd[1] = BSP_AHT21_INIT_PARAM_0;
    cmd[2] = BSP_AHT21_INIT_PARAM_1;

    if(BSP_IIC_Write(BSP_AHT21_I2C_ADDR_7BIT, cmd, sizeof(cmd)) != 0) {
        return -1;
    }

    HAL_Delay(BSP_AHT21_INIT_DELAY_MS);
    return 0;
}

/**
 * @brief 计算 AHT21 数据帧 CRC8，初值 0xFF，多项式 0x31。
 */
static uint8_t BSP_AHT21_CalcCrc8(const uint8_t * data, uint8_t len)
{
    uint8_t crc = 0xFFU;
    uint8_t i;
    uint8_t bit;

    if(data == NULL) {
        return 0U;
    }

    for(i = 0U; i < len; i++) {
        crc ^= data[i];
        for(bit = 0U; bit < 8U; bit++) {
            if((crc & 0x80U) != 0U) {
                crc = (uint8_t)((crc << 1) ^ 0x31U);
            } else {
                crc = (uint8_t)(crc << 1);
            }
        }
    }

    return crc;
}
