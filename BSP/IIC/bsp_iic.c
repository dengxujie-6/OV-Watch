/**
 * @file bsp_iic.c
 * @brief PB13/PB14 软件 IIC 总线驱动。
 */

#include "bsp_iic.h"

#include <stddef.h>

#include "stm32f4xx_hal.h"

#define BSP_IIC_SDA_PORT      GPIOB
#define BSP_IIC_SDA_PIN       GPIO_PIN_13
#define BSP_IIC_SCL_PORT      GPIOB
#define BSP_IIC_SCL_PIN       GPIO_PIN_14
#define BSP_IIC_ACK_TIMEOUT   1000U

static uint8_t iic_initialized;

static void BSP_IIC_DelayInit(void);
static void BSP_IIC_DelayUs(uint32_t us);
static void BSP_IIC_SDA_Out(void);
static void BSP_IIC_SDA_In(void);
static void BSP_IIC_WriteSDA(GPIO_PinState state);
static void BSP_IIC_WriteSCL(GPIO_PinState state);
static GPIO_PinState BSP_IIC_ReadSDA(void);
static void BSP_IIC_Start(void);
static void BSP_IIC_Stop(void);
static uint8_t BSP_IIC_SendByte(uint8_t data);
static uint8_t BSP_IIC_ReceiveByte(void);
static void BSP_IIC_Ack(void);
static void BSP_IIC_NoAck(void);
static uint8_t BSP_IIC_WaitAck(uint16_t timeout_limit);
static uint8_t BSP_IIC_IsAddressValid(uint8_t dev_addr_7bit);
static uint8_t BSP_IIC_WriteAddress(uint8_t dev_addr_7bit);
static uint8_t BSP_IIC_ReadAddress(uint8_t dev_addr_7bit);

/**
 * @brief 初始化 PB13/PB14 软件 IIC 总线。
 */
void BSP_IIC_Init(void)
{
    GPIO_InitTypeDef gpio_init = {0};

    __HAL_RCC_GPIOB_CLK_ENABLE();
    BSP_IIC_DelayInit();

    gpio_init.Pin = BSP_IIC_SDA_PIN | BSP_IIC_SCL_PIN;
    gpio_init.Mode = GPIO_MODE_OUTPUT_OD;
    gpio_init.Pull = GPIO_PULLUP;
    gpio_init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(GPIOB, &gpio_init);

    // I2C 空闲时 SDA 和 SCL 都释放为高电平。
    BSP_IIC_WriteSDA(GPIO_PIN_SET);
    BSP_IIC_WriteSCL(GPIO_PIN_SET);

    iic_initialized = 1U;
}

/**
 * @brief 反初始化 PB13/PB14 软件 IIC 总线。
 */
void BSP_IIC_DeInit(void)
{
    HAL_GPIO_DeInit(GPIOB, BSP_IIC_SDA_PIN | BSP_IIC_SCL_PIN);
    iic_initialized = 0U;
}

/**
 * @brief 探测IIC 总线上指定 7 位地址是否响应 ACK。
 */
int BSP_IIC_Probe(uint8_t dev_addr_7bit)
{
    int ret = 0;

    if(BSP_IIC_IsAddressValid(dev_addr_7bit) == 0U) {
        return -1;
    }

    if(iic_initialized == 0U) {
        BSP_IIC_Init();
    }

    BSP_IIC_Start();
    if(BSP_IIC_WriteAddress(dev_addr_7bit) == 0U) {
        ret = -2;
    }
    BSP_IIC_Stop();

    return ret;
}

/**
 * @brief 向IIC 总线写入连续原始字节。
 */
int BSP_IIC_Write(uint8_t dev_addr_7bit, const uint8_t * data, uint16_t len)
{
    uint16_t i;

    if((BSP_IIC_IsAddressValid(dev_addr_7bit) == 0U) || (data == NULL) || (len == 0U)) {
        return -1;
    }

    if(iic_initialized == 0U) {
        BSP_IIC_Init();
    }

    BSP_IIC_Start();
    if(BSP_IIC_WriteAddress(dev_addr_7bit) == 0U) {
        BSP_IIC_Stop();
        return -2;
    }

    for(i = 0U; i < len; i++) {
        if(BSP_IIC_SendByte(data[i]) == 0U) {
            BSP_IIC_Stop();
            return -3;
        }
    }

    BSP_IIC_Stop();
    return 0;
}

/**
 * @brief 从IIC 总线读取连续原始字节。
 */
int BSP_IIC_Read(uint8_t dev_addr_7bit, uint8_t * data, uint16_t len)
{
    uint16_t i;

    if((BSP_IIC_IsAddressValid(dev_addr_7bit) == 0U) || (data == NULL) || (len == 0U)) {
        return -1;
    }

    if(iic_initialized == 0U) {
        BSP_IIC_Init();
    }

    BSP_IIC_Start();
    if(BSP_IIC_ReadAddress(dev_addr_7bit) == 0U) {
        BSP_IIC_Stop();
        return -2;
    }

    for(i = 0U; i < len; i++) {
        data[i] = BSP_IIC_ReceiveByte();
        if((i + 1U) < len) {
            BSP_IIC_Ack();
        }
        else {
            BSP_IIC_NoAck();
        }
    }

    BSP_IIC_Stop();
    return 0;
}

/**
 * @brief 写入命令字节后使用重复起始读取数据。
 */
int BSP_IIC_WriteRead(uint8_t dev_addr_7bit,
                      const uint8_t * tx_data,
                      uint16_t tx_len,
                      uint8_t * rx_data,
                      uint16_t rx_len)
{
    uint16_t i;

    if((BSP_IIC_IsAddressValid(dev_addr_7bit) == 0U) ||
       (tx_data == NULL) ||
       (tx_len == 0U) ||
       (rx_data == NULL) ||
       (rx_len == 0U)) {
        return -1;
    }

    if(iic_initialized == 0U) {
        BSP_IIC_Init();
    }

    BSP_IIC_Start();
    if(BSP_IIC_WriteAddress(dev_addr_7bit) == 0U) {
        BSP_IIC_Stop();
        return -2;
    }

    for(i = 0U; i < tx_len; i++) {
        if(BSP_IIC_SendByte(tx_data[i]) == 0U) {
            BSP_IIC_Stop();
            return -3;
        }
    }

    // 写阶段结束后不发送 STOP，使用重复起始保持一次完整寄存器读事务。
    BSP_IIC_Start();
    if(BSP_IIC_ReadAddress(dev_addr_7bit) == 0U) {
        BSP_IIC_Stop();
        return -4;
    }

    for(i = 0U; i < rx_len; i++) {
        rx_data[i] = BSP_IIC_ReceiveByte();
        if((i + 1U) < rx_len) {
            BSP_IIC_Ack();
        }
        else {
            BSP_IIC_NoAck();
        }
    }

    BSP_IIC_Stop();
    return 0;
}

/**
 * @brief 写入器件 8 位寄存器的单字节值。
 */
int BSP_IIC_WriteReg(uint8_t dev_addr_7bit, uint8_t reg, uint8_t value)
{
    uint8_t buf[2];

    buf[0] = reg;
    buf[1] = value;

    return BSP_IIC_Write(dev_addr_7bit, buf, sizeof(buf));
}

/**
 * @brief 从器件 8 位寄存器地址连续读取数据。
 */
int BSP_IIC_ReadRegs(uint8_t dev_addr_7bit, uint8_t reg, uint8_t * data, uint16_t len)
{
    return BSP_IIC_WriteRead(dev_addr_7bit, &reg, 1U, data, len);
}

/**
 * @brief 初始化 DWT 周期计数器，用于微秒级软件 I2C 延时。
 */
static void BSP_IIC_DelayInit(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0U;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

/**
 * @brief 基于 DWT 的微秒延时。
 */
static void BSP_IIC_DelayUs(uint32_t us)
{
    uint32_t start = DWT->CYCCNT;
    uint32_t ticks = us * (SystemCoreClock / 1000000U);

    while((DWT->CYCCNT - start) < ticks) {
        // 等待 DWT 计数达到目标周期数。
    }
}

/**
 * @brief 将IIC SDA 配置为开漏输出。
 */
static void BSP_IIC_SDA_Out(void)
{
    GPIO_InitTypeDef gpio_init = {0};

    gpio_init.Pin = BSP_IIC_SDA_PIN;
    gpio_init.Mode = GPIO_MODE_OUTPUT_OD;
    gpio_init.Pull = GPIO_PULLUP;
    gpio_init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(BSP_IIC_SDA_PORT, &gpio_init);
}

/**
 * @brief 将IIC SDA 配置为输入，用于读取 ACK 或数据位。
 */
static void BSP_IIC_SDA_In(void)
{
    GPIO_InitTypeDef gpio_init = {0};

    gpio_init.Pin = BSP_IIC_SDA_PIN;
    gpio_init.Mode = GPIO_MODE_INPUT;
    gpio_init.Pull = GPIO_PULLUP;
    gpio_init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(BSP_IIC_SDA_PORT, &gpio_init);
}

/**
 * @brief 写 PB13 SDA 电平。
 */
static void BSP_IIC_WriteSDA(GPIO_PinState state)
{
    HAL_GPIO_WritePin(BSP_IIC_SDA_PORT, BSP_IIC_SDA_PIN, state);
}

/**
 * @brief 写 PB14 SCL 电平。
 */
static void BSP_IIC_WriteSCL(GPIO_PinState state)
{
    HAL_GPIO_WritePin(BSP_IIC_SCL_PORT, BSP_IIC_SCL_PIN, state);
}

/**
 * @brief 读取 PB13 SDA 电平。
 */
static GPIO_PinState BSP_IIC_ReadSDA(void)
{
    return HAL_GPIO_ReadPin(BSP_IIC_SDA_PORT, BSP_IIC_SDA_PIN);
}

/**
 * @brief 发送 I2C 起始条件。
 */
static void BSP_IIC_Start(void)
{
    BSP_IIC_SDA_Out();
    BSP_IIC_WriteSDA(GPIO_PIN_SET);
    BSP_IIC_DelayUs(1U);
    BSP_IIC_WriteSCL(GPIO_PIN_SET);
    BSP_IIC_DelayUs(5U);
    BSP_IIC_WriteSDA(GPIO_PIN_RESET);
    BSP_IIC_DelayUs(5U);
    BSP_IIC_WriteSCL(GPIO_PIN_RESET);
    BSP_IIC_DelayUs(2U);
}

/**
 * @brief 发送 I2C 停止条件。
 */
static void BSP_IIC_Stop(void)
{
    BSP_IIC_SDA_Out();
    BSP_IIC_WriteSCL(GPIO_PIN_RESET);
    BSP_IIC_DelayUs(2U);
    BSP_IIC_WriteSDA(GPIO_PIN_RESET);
    BSP_IIC_DelayUs(1U);
    BSP_IIC_WriteSCL(GPIO_PIN_SET);
    BSP_IIC_DelayUs(5U);
    BSP_IIC_WriteSDA(GPIO_PIN_SET);
    BSP_IIC_DelayUs(5U);
}

/**
 * @brief 发送 1 字节并等待从机 ACK。
 */
static uint8_t BSP_IIC_SendByte(uint8_t data)
{
    uint8_t bit;

    BSP_IIC_SDA_Out();

    for(bit = 0U; bit < 8U; bit++) {
        BSP_IIC_WriteSDA((data & 0x80U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
        data <<= 1;
        BSP_IIC_DelayUs(1U);
        BSP_IIC_WriteSCL(GPIO_PIN_SET);
        BSP_IIC_DelayUs(5U);
        BSP_IIC_WriteSCL(GPIO_PIN_RESET);
        BSP_IIC_DelayUs(2U);
    }

    return BSP_IIC_WaitAck(BSP_IIC_ACK_TIMEOUT);
}

/**
 * @brief 接收 1 字节。
 */
static uint8_t BSP_IIC_ReceiveByte(void)
{
    uint8_t bit;
    uint8_t data = 0U;

    BSP_IIC_SDA_In();

    for(bit = 0U; bit < 8U; bit++) {
        BSP_IIC_WriteSCL(GPIO_PIN_RESET);
        BSP_IIC_DelayUs(2U);
        BSP_IIC_WriteSCL(GPIO_PIN_SET);
        BSP_IIC_DelayUs(5U);

        data <<= 1;
        if(BSP_IIC_ReadSDA() == GPIO_PIN_SET) {
            data |= 0x01U;
        }
    }

    BSP_IIC_WriteSCL(GPIO_PIN_RESET);
    BSP_IIC_SDA_Out();
    BSP_IIC_DelayUs(2U);

    return data;
}

/**
 * @brief 主机发送 ACK。
 */
static void BSP_IIC_Ack(void)
{
    BSP_IIC_SDA_Out();
    BSP_IIC_WriteSDA(GPIO_PIN_RESET);
    BSP_IIC_DelayUs(2U);
    BSP_IIC_WriteSCL(GPIO_PIN_SET);
    BSP_IIC_DelayUs(5U);
    BSP_IIC_WriteSCL(GPIO_PIN_RESET);
    BSP_IIC_WriteSDA(GPIO_PIN_SET);
    BSP_IIC_DelayUs(2U);
}

/**
 * @brief 主机发送 NACK。
 */
static void BSP_IIC_NoAck(void)
{
    BSP_IIC_SDA_Out();
    BSP_IIC_WriteSDA(GPIO_PIN_SET);
    BSP_IIC_DelayUs(2U);
    BSP_IIC_WriteSCL(GPIO_PIN_SET);
    BSP_IIC_DelayUs(5U);
    BSP_IIC_WriteSCL(GPIO_PIN_RESET);
    BSP_IIC_DelayUs(2U);
}

/**
 * @brief 等待从机 ACK。
 */
static uint8_t BSP_IIC_WaitAck(uint16_t timeout_limit)
{
    uint16_t timeout = 0U;

    BSP_IIC_WriteSDA(GPIO_PIN_SET);
    BSP_IIC_SDA_In();
    BSP_IIC_DelayUs(1U);
    BSP_IIC_WriteSCL(GPIO_PIN_SET);
    BSP_IIC_DelayUs(1U);

    while(BSP_IIC_ReadSDA() != GPIO_PIN_RESET) {
        if(timeout++ > timeout_limit) {
            BSP_IIC_SDA_Out();
            return 0U;
        }
    }

    BSP_IIC_WriteSCL(GPIO_PIN_RESET);
    BSP_IIC_SDA_Out();
    BSP_IIC_DelayUs(2U);

    return 1U;
}

/**
 * @brief 检查地址是否保持在 7 位 I2C 地址范围内。
 */
static uint8_t BSP_IIC_IsAddressValid(uint8_t dev_addr_7bit)
{
    return (dev_addr_7bit <= 0x7FU) ? 1U : 0U;
}

/**
 * @brief 发送写方向地址字节。
 */
static uint8_t BSP_IIC_WriteAddress(uint8_t dev_addr_7bit)
{
    return BSP_IIC_SendByte((uint8_t)((dev_addr_7bit << 1) | 0U));
}

/**
 * @brief 发送读方向地址字节。
 */
static uint8_t BSP_IIC_ReadAddress(uint8_t dev_addr_7bit)
{
    return BSP_IIC_SendByte((uint8_t)((dev_addr_7bit << 1) | 1U));
}
