/**
 * @file bsp_prom_iic.c
 * @brief PA11/PA12 PROM 专用软件 I2C 总线驱动。
 */

#include "bsp_prom_iic.h"

#include <stddef.h>

#include "stm32f4xx_hal.h"

#define BSP_PROM_IIC_SDA_PORT      GPIOA
#define BSP_PROM_IIC_SDA_PIN       GPIO_PIN_11
#define BSP_PROM_IIC_SCL_PORT      GPIOA
#define BSP_PROM_IIC_SCL_PIN       GPIO_PIN_12
#define BSP_PROM_IIC_ACK_TIMEOUT   1000U

static uint8_t prom_iic_initialized;

static void BSP_PROM_IIC_DelayInit(void);
static void BSP_PROM_IIC_DelayUs(uint32_t us);
static void BSP_PROM_IIC_SDA_Out(void);
static void BSP_PROM_IIC_SDA_In(void);
static void BSP_PROM_IIC_WriteSDA(GPIO_PinState state);
static void BSP_PROM_IIC_WriteSCL(GPIO_PinState state);
static GPIO_PinState BSP_PROM_IIC_ReadSDA(void);
static void BSP_PROM_IIC_Start(void);
static void BSP_PROM_IIC_Stop(void);
static uint8_t BSP_PROM_IIC_SendByte(uint8_t data);
static uint8_t BSP_PROM_IIC_ReceiveByte(void);
static void BSP_PROM_IIC_Ack(void);
static void BSP_PROM_IIC_NoAck(void);
static uint8_t BSP_PROM_IIC_WaitAck(uint16_t timeout_limit);
static uint8_t BSP_PROM_IIC_IsAddressValid(uint8_t dev_addr_7bit);
static uint8_t BSP_PROM_IIC_WriteAddress(uint8_t dev_addr_7bit);
static uint8_t BSP_PROM_IIC_ReadAddress(uint8_t dev_addr_7bit);

/**
 * @brief 初始化 PA11/PA12 软件 I2C 总线。
 */
void BSP_PROM_IIC_Init(void)
{
    GPIO_InitTypeDef gpio_init = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    BSP_PROM_IIC_DelayInit();

    gpio_init.Pin = BSP_PROM_IIC_SDA_PIN | BSP_PROM_IIC_SCL_PIN;
    gpio_init.Mode = GPIO_MODE_OUTPUT_OD;
    gpio_init.Pull = GPIO_PULLUP;
    gpio_init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(GPIOA, &gpio_init);

    // I2C 空闲时 SDA 和 SCL 都释放为高电平。
    BSP_PROM_IIC_WriteSDA(GPIO_PIN_SET);
    BSP_PROM_IIC_WriteSCL(GPIO_PIN_SET);

    prom_iic_initialized = 1U;
}

/**
 * @brief 探测 PROM I2C 总线上的 7 位设备地址是否响应 ACK。
 */
int BSP_PROM_IIC_Probe(uint8_t dev_addr_7bit)
{
    int ret = 0;

    if(BSP_PROM_IIC_IsAddressValid(dev_addr_7bit) == 0U) {
        return -1;
    }

    if(prom_iic_initialized == 0U) {
        BSP_PROM_IIC_Init();
    }

    BSP_PROM_IIC_Start();
    if(BSP_PROM_IIC_WriteAddress(dev_addr_7bit) == 0U) {
        ret = -2;
    }
    BSP_PROM_IIC_Stop();

    return ret;
}

/**
 * @brief 向 PROM I2C 总线写入连续原始字节。
 */
int BSP_PROM_IIC_Write(uint8_t dev_addr_7bit, const uint8_t * data, uint16_t len)
{
    uint16_t i;

    if((BSP_PROM_IIC_IsAddressValid(dev_addr_7bit) == 0U) || (data == NULL) || (len == 0U)) {
        return -1;
    }

    if(prom_iic_initialized == 0U) {
        BSP_PROM_IIC_Init();
    }

    BSP_PROM_IIC_Start();
    if(BSP_PROM_IIC_WriteAddress(dev_addr_7bit) == 0U) {
        BSP_PROM_IIC_Stop();
        return -2;
    }

    for(i = 0U; i < len; i++) {
        if(BSP_PROM_IIC_SendByte(data[i]) == 0U) {
            BSP_PROM_IIC_Stop();
            return -3;
        }
    }

    BSP_PROM_IIC_Stop();
    return 0;
}

/**
 * @brief 从 PROM I2C 总线读取连续原始字节。
 */
int BSP_PROM_IIC_Read(uint8_t dev_addr_7bit, uint8_t * data, uint16_t len)
{
    uint16_t i;

    if((BSP_PROM_IIC_IsAddressValid(dev_addr_7bit) == 0U) || (data == NULL) || (len == 0U)) {
        return -1;
    }

    if(prom_iic_initialized == 0U) {
        BSP_PROM_IIC_Init();
    }

    BSP_PROM_IIC_Start();
    if(BSP_PROM_IIC_ReadAddress(dev_addr_7bit) == 0U) {
        BSP_PROM_IIC_Stop();
        return -2;
    }

    for(i = 0U; i < len; i++) {
        data[i] = BSP_PROM_IIC_ReceiveByte();
        if((i + 1U) < len) {
            BSP_PROM_IIC_Ack();
        }
        else {
            BSP_PROM_IIC_NoAck();
        }
    }

    BSP_PROM_IIC_Stop();
    return 0;
}

/**
 * @brief 写入命令字节后使用重复起始读取数据。
 */
int BSP_PROM_IIC_WriteRead(uint8_t dev_addr_7bit,
                           const uint8_t * tx_data,
                           uint16_t tx_len,
                           uint8_t * rx_data,
                           uint16_t rx_len)
{
    uint16_t i;

    if((BSP_PROM_IIC_IsAddressValid(dev_addr_7bit) == 0U) ||
       (tx_data == NULL) ||
       (tx_len == 0U) ||
       (rx_data == NULL) ||
       (rx_len == 0U)) {
        return -1;
    }

    if(prom_iic_initialized == 0U) {
        BSP_PROM_IIC_Init();
    }

    BSP_PROM_IIC_Start();
    if(BSP_PROM_IIC_WriteAddress(dev_addr_7bit) == 0U) {
        BSP_PROM_IIC_Stop();
        return -2;
    }

    for(i = 0U; i < tx_len; i++) {
        if(BSP_PROM_IIC_SendByte(tx_data[i]) == 0U) {
            BSP_PROM_IIC_Stop();
            return -3;
        }
    }

    BSP_PROM_IIC_Start();
    if(BSP_PROM_IIC_ReadAddress(dev_addr_7bit) == 0U) {
        BSP_PROM_IIC_Stop();
        return -4;
    }

    for(i = 0U; i < rx_len; i++) {
        rx_data[i] = BSP_PROM_IIC_ReceiveByte();
        if((i + 1U) < rx_len) {
            BSP_PROM_IIC_Ack();
        }
        else {
            BSP_PROM_IIC_NoAck();
        }
    }

    BSP_PROM_IIC_Stop();
    return 0;
}

/**
 * @brief 初始化 DWT 周期计数器，用于微秒级软件 I2C 延时。
 */
static void BSP_PROM_IIC_DelayInit(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0U;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

/**
 * @brief 基于 DWT 的微秒延时。
 */
static void BSP_PROM_IIC_DelayUs(uint32_t us)
{
    uint32_t start = DWT->CYCCNT;
    uint32_t ticks = us * (SystemCoreClock / 1000000U);

    while((DWT->CYCCNT - start) < ticks) {
        // 等待 DWT 计数达到目标周期数。
    }
}

/**
 * @brief 将 PROM SDA 配置为开漏输出。
 */
static void BSP_PROM_IIC_SDA_Out(void)
{
    GPIO_InitTypeDef gpio_init = {0};

    gpio_init.Pin = BSP_PROM_IIC_SDA_PIN;
    gpio_init.Mode = GPIO_MODE_OUTPUT_OD;
    gpio_init.Pull = GPIO_PULLUP;
    gpio_init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(BSP_PROM_IIC_SDA_PORT, &gpio_init);
}

/**
 * @brief 将 PROM SDA 配置为输入，用于读取 ACK 或数据位。
 */
static void BSP_PROM_IIC_SDA_In(void)
{
    GPIO_InitTypeDef gpio_init = {0};

    gpio_init.Pin = BSP_PROM_IIC_SDA_PIN;
    gpio_init.Mode = GPIO_MODE_INPUT;
    gpio_init.Pull = GPIO_PULLUP;
    gpio_init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(BSP_PROM_IIC_SDA_PORT, &gpio_init);
}

/**
 * @brief 写 PA11 SDA 电平。
 */
static void BSP_PROM_IIC_WriteSDA(GPIO_PinState state)
{
    HAL_GPIO_WritePin(BSP_PROM_IIC_SDA_PORT, BSP_PROM_IIC_SDA_PIN, state);
}

/**
 * @brief 写 PA12 SCL 电平。
 */
static void BSP_PROM_IIC_WriteSCL(GPIO_PinState state)
{
    HAL_GPIO_WritePin(BSP_PROM_IIC_SCL_PORT, BSP_PROM_IIC_SCL_PIN, state);
}

/**
 * @brief 读取 PA11 SDA 电平。
 */
static GPIO_PinState BSP_PROM_IIC_ReadSDA(void)
{
    return HAL_GPIO_ReadPin(BSP_PROM_IIC_SDA_PORT, BSP_PROM_IIC_SDA_PIN);
}

/**
 * @brief 发送 I2C 起始条件。
 */
static void BSP_PROM_IIC_Start(void)
{
    BSP_PROM_IIC_SDA_Out();
    BSP_PROM_IIC_WriteSDA(GPIO_PIN_SET);
    BSP_PROM_IIC_DelayUs(1U);
    BSP_PROM_IIC_WriteSCL(GPIO_PIN_SET);
    BSP_PROM_IIC_DelayUs(5U);
    BSP_PROM_IIC_WriteSDA(GPIO_PIN_RESET);
    BSP_PROM_IIC_DelayUs(5U);
    BSP_PROM_IIC_WriteSCL(GPIO_PIN_RESET);
    BSP_PROM_IIC_DelayUs(2U);
}

/**
 * @brief 发送 I2C 停止条件。
 */
static void BSP_PROM_IIC_Stop(void)
{
    BSP_PROM_IIC_SDA_Out();
    BSP_PROM_IIC_WriteSCL(GPIO_PIN_RESET);
    BSP_PROM_IIC_DelayUs(2U);
    BSP_PROM_IIC_WriteSDA(GPIO_PIN_RESET);
    BSP_PROM_IIC_DelayUs(1U);
    BSP_PROM_IIC_WriteSCL(GPIO_PIN_SET);
    BSP_PROM_IIC_DelayUs(5U);
    BSP_PROM_IIC_WriteSDA(GPIO_PIN_SET);
    BSP_PROM_IIC_DelayUs(5U);
}

/**
 * @brief 发送 1 字节并等待从机 ACK。
 */
static uint8_t BSP_PROM_IIC_SendByte(uint8_t data)
{
    uint8_t bit;

    BSP_PROM_IIC_SDA_Out();

    for(bit = 0U; bit < 8U; bit++) {
        BSP_PROM_IIC_WriteSDA((data & 0x80U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
        data <<= 1;
        BSP_PROM_IIC_DelayUs(1U);
        BSP_PROM_IIC_WriteSCL(GPIO_PIN_SET);
        BSP_PROM_IIC_DelayUs(5U);
        BSP_PROM_IIC_WriteSCL(GPIO_PIN_RESET);
        BSP_PROM_IIC_DelayUs(2U);
    }

    return BSP_PROM_IIC_WaitAck(BSP_PROM_IIC_ACK_TIMEOUT);
}

/**
 * @brief 接收 1 字节。
 */
static uint8_t BSP_PROM_IIC_ReceiveByte(void)
{
    uint8_t bit;
    uint8_t data = 0U;

    BSP_PROM_IIC_SDA_In();

    for(bit = 0U; bit < 8U; bit++) {
        BSP_PROM_IIC_WriteSCL(GPIO_PIN_RESET);
        BSP_PROM_IIC_DelayUs(2U);
        BSP_PROM_IIC_WriteSCL(GPIO_PIN_SET);
        BSP_PROM_IIC_DelayUs(5U);

        data <<= 1;
        if(BSP_PROM_IIC_ReadSDA() == GPIO_PIN_SET) {
            data |= 0x01U;
        }
    }

    BSP_PROM_IIC_WriteSCL(GPIO_PIN_RESET);
    BSP_PROM_IIC_SDA_Out();
    BSP_PROM_IIC_DelayUs(2U);

    return data;
}

/**
 * @brief 主机发送 ACK。
 */
static void BSP_PROM_IIC_Ack(void)
{
    BSP_PROM_IIC_SDA_Out();
    BSP_PROM_IIC_WriteSDA(GPIO_PIN_RESET);
    BSP_PROM_IIC_DelayUs(2U);
    BSP_PROM_IIC_WriteSCL(GPIO_PIN_SET);
    BSP_PROM_IIC_DelayUs(5U);
    BSP_PROM_IIC_WriteSCL(GPIO_PIN_RESET);
    BSP_PROM_IIC_WriteSDA(GPIO_PIN_SET);
    BSP_PROM_IIC_DelayUs(2U);
}

/**
 * @brief 主机发送 NACK。
 */
static void BSP_PROM_IIC_NoAck(void)
{
    BSP_PROM_IIC_SDA_Out();
    BSP_PROM_IIC_WriteSDA(GPIO_PIN_SET);
    BSP_PROM_IIC_DelayUs(2U);
    BSP_PROM_IIC_WriteSCL(GPIO_PIN_SET);
    BSP_PROM_IIC_DelayUs(5U);
    BSP_PROM_IIC_WriteSCL(GPIO_PIN_RESET);
    BSP_PROM_IIC_DelayUs(2U);
}

/**
 * @brief 等待从机 ACK。
 */
static uint8_t BSP_PROM_IIC_WaitAck(uint16_t timeout_limit)
{
    uint16_t timeout = 0U;

    BSP_PROM_IIC_WriteSDA(GPIO_PIN_SET);
    BSP_PROM_IIC_SDA_In();
    BSP_PROM_IIC_DelayUs(1U);
    BSP_PROM_IIC_WriteSCL(GPIO_PIN_SET);
    BSP_PROM_IIC_DelayUs(1U);

    while(BSP_PROM_IIC_ReadSDA() != GPIO_PIN_RESET) {
        if(timeout++ > timeout_limit) {
            BSP_PROM_IIC_SDA_Out();
            return 0U;
        }
    }

    BSP_PROM_IIC_WriteSCL(GPIO_PIN_RESET);
    BSP_PROM_IIC_SDA_Out();
    BSP_PROM_IIC_DelayUs(2U);

    return 1U;
}

/**
 * @brief 检查地址是否保持在 7 位 I2C 地址范围内。
 */
static uint8_t BSP_PROM_IIC_IsAddressValid(uint8_t dev_addr_7bit)
{
    return (dev_addr_7bit <= 0x7FU) ? 1U : 0U;
}

/**
 * @brief 发送写方向地址字节。
 */
static uint8_t BSP_PROM_IIC_WriteAddress(uint8_t dev_addr_7bit)
{
    return BSP_PROM_IIC_SendByte((uint8_t)((dev_addr_7bit << 1) | 0U));
}

/**
 * @brief 发送读方向地址字节。
 */
static uint8_t BSP_PROM_IIC_ReadAddress(uint8_t dev_addr_7bit)
{
    return BSP_PROM_IIC_SendByte((uint8_t)((dev_addr_7bit << 1) | 1U));
}
