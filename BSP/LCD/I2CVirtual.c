/**
 * @file I2CVirtual.c
 * @brief 基于 HAL GPIO 的软件模拟 I2C 实现。
 */

#include "I2CVirtual.h"

#ifdef I2C_VIRTUAL

static uint16_t I2C_Virtual_SDA_PinMask;
static uint16_t I2C_Virtual_SCL_PinMask;
static GPIO_TypeDef * I2C_Virtual_SDA_Port;
static GPIO_TypeDef * I2C_Virtual_SCL_Port;

uint8_t I2C_Virtual_ack;

static GPIO_TypeDef * I2C_Virtual_GetPort(char port);
static void I2C_Virtual_EnableClock(char port);
static void I2C_Virtual_DelayInit(void);
static void I2C_Virtual_DelayUs(uint32_t us);
static void I2C_Virtual_WriteSDA(GPIO_PinState state);
static void I2C_Virtual_WriteSCL(GPIO_PinState state);
static GPIO_PinState I2C_Virtual_ReadSDA(void);

/**
 * @brief 配置模拟 I2C 的 GPIO 引脚。
 *
 * SDA 配置为开漏输出，便于从机拉低 ACK；SCL 也使用开漏输出，符合 I2C 总线习惯。
 *
 * @param SDA_Port SDA 端口字符，例如 'B'。
 * @param SDA_Pin SDA 引脚序号，例如 4。
 * @param SCL_Port SCL 端口字符，例如 'B'。
 * @param SCL_Pin SCL 引脚序号，例如 6。
 */
void I2C_Virtual_ConfigPort(char SDA_Port, uint8_t SDA_Pin, char SCL_Port, uint8_t SCL_Pin)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};

    I2C_Virtual_EnableClock(SDA_Port);
    I2C_Virtual_EnableClock(SCL_Port);
    I2C_Virtual_DelayInit();

    I2C_Virtual_SDA_Port = I2C_Virtual_GetPort(SDA_Port);
    I2C_Virtual_SCL_Port = I2C_Virtual_GetPort(SCL_Port);
    I2C_Virtual_SDA_PinMask = (uint16_t)(1U << SDA_Pin);
    I2C_Virtual_SCL_PinMask = (uint16_t)(1U << SCL_Pin);

    GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStructure.Pull = GPIO_PULLUP;
    GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_VERY_HIGH;

    GPIO_InitStructure.Pin = I2C_Virtual_SDA_PinMask;
    HAL_GPIO_Init(I2C_Virtual_SDA_Port, &GPIO_InitStructure);

    GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStructure.Pull = GPIO_NOPULL;
    GPIO_InitStructure.Pin = I2C_Virtual_SCL_PinMask;
    HAL_GPIO_Init(I2C_Virtual_SCL_Port, &GPIO_InitStructure);
}

/**
 * @brief 切换当前软件 I2C 总线。
 *
 * @param SDA_Port SDA 端口字符。
 * @param SDA_Pin SDA 引脚序号。
 * @param SCL_Port SCL 端口字符。
 * @param SCL_Pin SCL 引脚序号。
 */
void I2C_Virtual_SwitchBus(char SDA_Port, uint8_t SDA_Pin, char SCL_Port, uint8_t SCL_Pin)
{
    I2C_Virtual_SDA_Port = I2C_Virtual_GetPort(SDA_Port);
    I2C_Virtual_SCL_Port = I2C_Virtual_GetPort(SCL_Port);
    I2C_Virtual_SDA_PinMask = (uint16_t)(1U << SDA_Pin);
    I2C_Virtual_SCL_PinMask = (uint16_t)(1U << SCL_Pin);
}

/**
 * @brief 将 SDA 切换为开漏输出。
 */
void I2C_Virtual_SetSDA_Out(void)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};

    GPIO_InitStructure.Pin = I2C_Virtual_SDA_PinMask;
    GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStructure.Pull = GPIO_PULLUP;
    GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(I2C_Virtual_SDA_Port, &GPIO_InitStructure);
}

/**
 * @brief 将 SDA 切换为输入，用于读取 ACK 或数据位。
 */
void I2C_Virtual_SetSDA_In(void)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};

    GPIO_InitStructure.Pin = I2C_Virtual_SDA_PinMask;
    GPIO_InitStructure.Mode = GPIO_MODE_INPUT;
    GPIO_InitStructure.Pull = GPIO_PULLUP;
    GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(I2C_Virtual_SDA_Port, &GPIO_InitStructure);
}

/**
 * @brief 初始化模拟 I2C 总线空闲电平。
 */
void I2C_Virtual_Init(void)
{
    I2C_Virtual_WriteSCL(GPIO_PIN_SET);
    I2C_Virtual_SetSDA_Out();
    I2C_Virtual_WriteSDA(GPIO_PIN_SET);
}

/**
 * @brief 发送 I2C 起始条件。
 */
void I2C_Virtual_Start(void)
{
    I2C_Virtual_SetSDA_Out();
    I2C_Virtual_WriteSDA(GPIO_PIN_SET);
    I2C_Virtual_DelayUs(1);
    I2C_Virtual_WriteSCL(GPIO_PIN_SET);
    I2C_Virtual_DelayUs(5);
    I2C_Virtual_WriteSDA(GPIO_PIN_RESET);
    I2C_Virtual_DelayUs(5);
    I2C_Virtual_WriteSCL(GPIO_PIN_RESET);
    I2C_Virtual_DelayUs(2);
}

/**
 * @brief 发送 I2C 停止条件。
 */
void I2C_Virtual_Stop(void)
{
    I2C_Virtual_SetSDA_Out();
    I2C_Virtual_WriteSCL(GPIO_PIN_RESET);
    I2C_Virtual_DelayUs(2);
    I2C_Virtual_WriteSDA(GPIO_PIN_RESET);
    I2C_Virtual_DelayUs(1);
    I2C_Virtual_WriteSCL(GPIO_PIN_SET);
    I2C_Virtual_DelayUs(5);
    I2C_Virtual_WriteSDA(GPIO_PIN_SET);
    I2C_Virtual_DelayUs(5);
}

/**
 * @brief 发送 1 字节，并等待从机 ACK。
 *
 * @param c 要发送的数据。
 * @return 1 表示收到 ACK，0 表示超时或无 ACK。
 */
uint8_t I2C_Virtual_SendByte(uint8_t c)
{
    uint8_t bit;

    I2C_Virtual_SetSDA_Out();

    for(bit = 0U; bit < 8U; bit++) {
        I2C_Virtual_WriteSDA((c & 0x80U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
        c <<= 1;
        I2C_Virtual_DelayUs(1);
        I2C_Virtual_WriteSCL(GPIO_PIN_SET);
        I2C_Virtual_DelayUs(5);
        I2C_Virtual_WriteSCL(GPIO_PIN_RESET);
        I2C_Virtual_DelayUs(2);
    }

    I2C_Virtual_WaitAck(1000U);
    return I2C_Virtual_ack;
}

/**
 * @brief 接收 1 字节。
 *
 * @return 接收到的数据。
 */
uint8_t I2C_Virtual_RcvByte(void)
{
    uint8_t bit;
    uint8_t data = 0U;

    I2C_Virtual_SetSDA_In();

    for(bit = 0U; bit < 8U; bit++) {
        I2C_Virtual_WriteSCL(GPIO_PIN_RESET);
        I2C_Virtual_DelayUs(2);
        I2C_Virtual_WriteSCL(GPIO_PIN_SET);
        I2C_Virtual_DelayUs(5);

        data <<= 1;
        if(I2C_Virtual_ReadSDA() == GPIO_PIN_SET) {
            data |= 0x01U;
        }
    }

    I2C_Virtual_WriteSCL(GPIO_PIN_RESET);
    I2C_Virtual_SetSDA_Out();
    I2C_Virtual_DelayUs(2);

    return data;
}

/**
 * @brief 主机发送 ACK。
 */
void I2C_Virtual_Ack(void)
{
    I2C_Virtual_SetSDA_Out();
    I2C_Virtual_WriteSDA(GPIO_PIN_RESET);
    I2C_Virtual_DelayUs(2);
    I2C_Virtual_WriteSCL(GPIO_PIN_SET);
    I2C_Virtual_DelayUs(5);
    I2C_Virtual_WriteSCL(GPIO_PIN_RESET);
    I2C_Virtual_WriteSDA(GPIO_PIN_SET);
    I2C_Virtual_DelayUs(2);
}

/**
 * @brief 主机发送 NACK。
 */
void I2C_Virtual_NoAck(void)
{
    I2C_Virtual_SetSDA_Out();
    I2C_Virtual_WriteSDA(GPIO_PIN_SET);
    I2C_Virtual_DelayUs(2);
    I2C_Virtual_WriteSCL(GPIO_PIN_SET);
    I2C_Virtual_DelayUs(5);
    I2C_Virtual_WriteSCL(GPIO_PIN_RESET);
    I2C_Virtual_DelayUs(2);
}

/**
 * @brief 等待从机 ACK。
 *
 * @param time 超时计数，单位不是严格时间，只用于避免死等。
 */
void I2C_Virtual_WaitAck(uint16_t time)
{
    uint16_t timeout = 0U;

    I2C_Virtual_WriteSDA(GPIO_PIN_SET);
    I2C_Virtual_SetSDA_In();
    I2C_Virtual_DelayUs(1);
    I2C_Virtual_WriteSCL(GPIO_PIN_SET);
    I2C_Virtual_DelayUs(1);

    while(I2C_Virtual_ReadSDA() != GPIO_PIN_RESET) {
        if(timeout++ > time) {
            I2C_Virtual_ack = 0U;
            I2C_Virtual_SetSDA_Out();
            I2C_Virtual_Stop();
            return;
        }
    }

    I2C_Virtual_ack = 1U;
    I2C_Virtual_WriteSCL(GPIO_PIN_RESET);
    I2C_Virtual_SetSDA_Out();
    I2C_Virtual_DelayUs(2);
}

static GPIO_TypeDef * I2C_Virtual_GetPort(char port)
{
    switch(port) {
        case 'A': return GPIOA;
        case 'B': return GPIOB;
        case 'C': return GPIOC;
#ifdef GPIOD
        case 'D': return GPIOD;
#endif
#ifdef GPIOE
        case 'E': return GPIOE;
#endif
#ifdef GPIOF
        case 'F': return GPIOF;
#endif
#ifdef GPIOG
        case 'G': return GPIOG;
#endif
        default: return GPIOB;
    }
}

static void I2C_Virtual_EnableClock(char port)
{
    switch(port) {
        case 'A':
            __HAL_RCC_GPIOA_CLK_ENABLE();
            break;
        case 'B':
            __HAL_RCC_GPIOB_CLK_ENABLE();
            break;
        case 'C':
            __HAL_RCC_GPIOC_CLK_ENABLE();
            break;
#ifdef __HAL_RCC_GPIOD_CLK_ENABLE
        case 'D':
            __HAL_RCC_GPIOD_CLK_ENABLE();
            break;
#endif
#ifdef __HAL_RCC_GPIOE_CLK_ENABLE
        case 'E':
            __HAL_RCC_GPIOE_CLK_ENABLE();
            break;
#endif
#ifdef __HAL_RCC_GPIOF_CLK_ENABLE
        case 'F':
            __HAL_RCC_GPIOF_CLK_ENABLE();
            break;
#endif
#ifdef __HAL_RCC_GPIOG_CLK_ENABLE
        case 'G':
            __HAL_RCC_GPIOG_CLK_ENABLE();
            break;
#endif
        default:
            __HAL_RCC_GPIOB_CLK_ENABLE();
            break;
    }
}

static void I2C_Virtual_DelayInit(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0U;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

static void I2C_Virtual_DelayUs(uint32_t us)
{
    uint32_t start = DWT->CYCCNT;
    uint32_t ticks = us * (SystemCoreClock / 1000000U);

    while((DWT->CYCCNT - start) < ticks) {
        // 等待 DWT 计数达到指定微秒数。
    }
}

static void I2C_Virtual_WriteSDA(GPIO_PinState state)
{
    HAL_GPIO_WritePin(I2C_Virtual_SDA_Port, I2C_Virtual_SDA_PinMask, state);
}

static void I2C_Virtual_WriteSCL(GPIO_PinState state)
{
    HAL_GPIO_WritePin(I2C_Virtual_SCL_Port, I2C_Virtual_SCL_PinMask, state);
}

static GPIO_PinState I2C_Virtual_ReadSDA(void)
{
    return HAL_GPIO_ReadPin(I2C_Virtual_SDA_Port, I2C_Virtual_SDA_PinMask);
}

#endif /* I2C_VIRTUAL */
