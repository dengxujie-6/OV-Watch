#ifndef __I2C_H__
#define __I2C_H__

#include <stdint.h>

#include "stm32f4xx_hal.h"

#define I2C_VIRTUAL  // 定义模拟 IIC

#ifdef I2C_VIRTUAL

typedef struct
{
	char SclPort;
	uint8_t SclPin;
	char SdaPort;
	uint8_t SdaPin;
} I2CPort_Struct;

typedef struct
{
	//I2C从机设备的端口
	I2CPort_Struct port;
	/*
	 * I2C设备的写地址 = I2C设备地址 << 1 + 0
	 * I2C设备的读地址 = (I2C设备地址 << 1) + 1
	 */
	//I2C从机设备地址（器件地址占7位，D7~D1）
	uint8_t address;
	//I2C从机设备读写状态（0：写，1：读）
//	uint8_t rwStatus;
	//I2C从机设备的状态（0：错误或不存在，1：正常）
	uint8_t devStatus;
} I2CDevice_Struct;

extern uint8_t I2C_Virtual_ack;

void I2C_Virtual_ConfigPort(char SDA_Port, uint8_t SDA_Pin, char SCL_Port, uint8_t SCL_Pin);
void I2C_Virtual_SwitchBus(char SDA_Port, uint8_t SDA_Pin, char SCL_Port, uint8_t SCL_Pin);
void I2C_Virtual_SetSDA_Out(void);
void I2C_Virtual_SetSDA_In(void);
void I2C_Virtual_Init(void);
void I2C_Virtual_Start(void);
void I2C_Virtual_Stop(void);
uint8_t I2C_Virtual_SendByte(uint8_t c);
uint8_t I2C_Virtual_RcvByte(void);
void I2C_Virtual_Ack(void);
void I2C_Virtual_NoAck(void);
void I2C_Virtual_WaitAck(uint16_t time);

#endif

#endif

