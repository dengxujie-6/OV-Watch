#include "iic_hal.h"

static void delay_us(uint32_t us);
static void SDA_Input_Mode(iic_bus_t *bus);
static void SDA_Output_Mode(iic_bus_t *bus);
static void SDA_Output(iic_bus_t *bus, uint16_t val);
static void SCL_Output(iic_bus_t *bus, uint16_t val);
static uint8_t SDA_Input(iic_bus_t *bus);

/**
 * @brief 将 SDA 配置为输入模式，用于读取 ACK 或数据位。
 */
static void SDA_Input_Mode(iic_bus_t *bus)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};

    GPIO_InitStructure.Pin = bus->IIC_SDA_PIN;
    GPIO_InitStructure.Mode = GPIO_MODE_INPUT;
    GPIO_InitStructure.Pull = GPIO_PULLUP;
    GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(bus->IIC_SDA_PORT, &GPIO_InitStructure);
}

/**
 * @brief 将 SDA 配置为开漏输出模式。
 */
static void SDA_Output_Mode(iic_bus_t *bus)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};

    GPIO_InitStructure.Pin = bus->IIC_SDA_PIN;
    GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStructure.Pull = GPIO_PULLUP;
    GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(bus->IIC_SDA_PORT, &GPIO_InitStructure);
}

static void SDA_Output(iic_bus_t *bus, uint16_t val)
{
    if(val != 0U) {
        bus->IIC_SDA_PORT->BSRR = bus->IIC_SDA_PIN;
    }
    else {
        bus->IIC_SDA_PORT->BSRR = (uint32_t)bus->IIC_SDA_PIN << 16U;
    }
}

static void SCL_Output(iic_bus_t *bus, uint16_t val)
{
    if(val != 0U) {
        bus->IIC_SCL_PORT->BSRR = bus->IIC_SCL_PIN;
    }
    else {
        bus->IIC_SCL_PORT->BSRR = (uint32_t)bus->IIC_SCL_PIN << 16U;
    }
}

static uint8_t SDA_Input(iic_bus_t *bus)
{
    return (HAL_GPIO_ReadPin(bus->IIC_SDA_PORT, bus->IIC_SDA_PIN) == GPIO_PIN_SET) ? 1U : 0U;
}

/**
 * @brief 发送 IIC 起始信号。
 */
void IICStart(iic_bus_t *bus)
{
    SDA_Output_Mode(bus);
    SDA_Output(bus, 1U);
    delay_us(2U);
    SCL_Output(bus, 1U);
    delay_us(1U);
    SDA_Output(bus, 0U);
    delay_us(1U);
    SCL_Output(bus, 0U);
    delay_us(1U);
}

/**
 * @brief 发送 IIC 停止信号。
 */
void IICStop(iic_bus_t *bus)
{
    SDA_Output_Mode(bus);
    SCL_Output(bus, 0U);
    delay_us(2U);
    SDA_Output(bus, 0U);
    delay_us(1U);
    SCL_Output(bus, 1U);
    delay_us(1U);
    SDA_Output(bus, 1U);
    delay_us(1U);
}

/**
 * @brief 等待从机 ACK。
 */
unsigned char IICWaitAck(iic_bus_t *bus)
{
    unsigned short cErrTime = 50U;

    SDA_Input_Mode(bus);
    SCL_Output(bus, 1U);

    while(SDA_Input(bus) != 0U) {
        cErrTime--;
        delay_us(1U);
        if(cErrTime == 0U) {
            SDA_Output_Mode(bus);
            IICStop(bus);
            return ERROR;
        }
    }

    SDA_Output_Mode(bus);
    SCL_Output(bus, 0U);
    delay_us(2U);
    return SUCCESS;
}

void IICSendAck(iic_bus_t *bus)
{
    SDA_Output_Mode(bus);
    SDA_Output(bus, 0U);
    delay_us(1U);
    SCL_Output(bus, 1U);
    delay_us(1U);
    SCL_Output(bus, 0U);
    delay_us(1U);
}

void IICSendNotAck(iic_bus_t *bus)
{
    SDA_Output_Mode(bus);
    SDA_Output(bus, 1U);
    delay_us(1U);
    SCL_Output(bus, 1U);
    delay_us(1U);
    SCL_Output(bus, 0U);
    delay_us(2U);
}

/**
 * @brief 发送 1 字节数据。
 */
void IICSendByte(iic_bus_t *bus, unsigned char cSendByte)
{
    unsigned char i = 8U;

    SDA_Output_Mode(bus);
    while(i-- != 0U) {
        SCL_Output(bus, 0U);
        delay_us(2U);
        SDA_Output(bus, (uint16_t)(cSendByte & 0x80U));
        delay_us(1U);
        cSendByte <<= 1;
        delay_us(1U);
        SCL_Output(bus, 1U);
        delay_us(1U);
    }

    SCL_Output(bus, 0U);
    delay_us(2U);
}

/**
 * @brief 接收 1 字节数据。
 */
unsigned char IICReceiveByte(iic_bus_t *bus)
{
    unsigned char i = 8U;
    unsigned char cR_Byte = 0U;

    SDA_Input_Mode(bus);
    while(i-- != 0U) {
        cR_Byte <<= 1;
        SCL_Output(bus, 0U);
        delay_us(2U);
        SCL_Output(bus, 1U);
        delay_us(1U);
        cR_Byte |= SDA_Input(bus);
    }

    SCL_Output(bus, 0U);
    SDA_Output_Mode(bus);
    return cR_Byte;
}

uint8_t IIC_Write_One_Byte(iic_bus_t *bus, uint8_t daddr, uint8_t reg, uint8_t data)
{
    IICStart(bus);

    IICSendByte(bus, (uint8_t)(daddr << 1));
    if(IICWaitAck(bus) != SUCCESS) {
        IICStop(bus);
        return 1U;
    }

    IICSendByte(bus, reg);
    IICWaitAck(bus);
    IICSendByte(bus, data);
    IICWaitAck(bus);
    IICStop(bus);
    delay_us(1U);
    return 0U;
}

uint8_t IIC_Write_Multi_Byte(iic_bus_t *bus, uint8_t daddr, uint8_t reg, uint8_t length, uint8_t buff[])
{
    uint8_t i;

    IICStart(bus);
    IICSendByte(bus, (uint8_t)(daddr << 1));
    if(IICWaitAck(bus) != SUCCESS) {
        IICStop(bus);
        return 1U;
    }

    IICSendByte(bus, reg);
    IICWaitAck(bus);
    for(i = 0U; i < length; i++) {
        IICSendByte(bus, buff[i]);
        IICWaitAck(bus);
    }

    IICStop(bus);
    delay_us(1U);
    return 0U;
}

unsigned char IIC_Read_One_Byte(iic_bus_t *bus, uint8_t daddr, uint8_t reg)
{
    unsigned char dat;

    IICStart(bus);
    IICSendByte(bus, (uint8_t)(daddr << 1));
    IICWaitAck(bus);
    IICSendByte(bus, reg);
    IICWaitAck(bus);

    IICStart(bus);
    IICSendByte(bus, (uint8_t)((daddr << 1) + 1U));
    IICWaitAck(bus);
    dat = IICReceiveByte(bus);
    IICSendNotAck(bus);
    IICStop(bus);
    return dat;
}

uint8_t IIC_Read_Multi_Byte(iic_bus_t *bus, uint8_t daddr, uint8_t reg, uint8_t length, uint8_t buff[])
{
    uint8_t i;

    IICStart(bus);
    IICSendByte(bus, (uint8_t)(daddr << 1));
    if(IICWaitAck(bus) != SUCCESS) {
        IICStop(bus);
        return 1U;
    }

    IICSendByte(bus, reg);
    IICWaitAck(bus);

    IICStart(bus);
    IICSendByte(bus, (uint8_t)((daddr << 1) + 1U));
    IICWaitAck(bus);
    for(i = 0U; i < length; i++) {
        buff[i] = IICReceiveByte(bus);
        if(i < (uint8_t)(length - 1U)) {
            IICSendAck(bus);
        }
    }

    IICSendNotAck(bus);
    IICStop(bus);
    return 0U;
}

/**
 * @brief 初始化软件 IIC 总线引脚。
 */
void IICInit(iic_bus_t *bus)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};

    GPIO_InitStructure.Pin = bus->IIC_SDA_PIN;
    GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStructure.Pull = GPIO_PULLUP;
    GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(bus->IIC_SDA_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.Pin = bus->IIC_SCL_PIN;
    GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStructure.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(bus->IIC_SCL_PORT, &GPIO_InitStructure);

    SDA_Output(bus, 1U);
    SCL_Output(bus, 1U);
}

static void delay_us(uint32_t us)
{
    uint32_t start;
    uint32_t ticks;

    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    start = DWT->CYCCNT;
    ticks = us * (SystemCoreClock / 1000000U);

    while((DWT->CYCCNT - start) < ticks) {
        // 等待 DWT 计数达到指定微秒数。
    }
}
