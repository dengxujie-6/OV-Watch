/**
 * @file st7789v.h
 * @author Renato Freitas (freitas-renato@outlook.com)
 * @brief ST7789V LCD 驱动函数声明和常量定义
 * @version 0.1
 * @date 2021-03-24
 * 
 * @note 本文件沿用 STMicroelectronics Nucleo/Discovery 板卡中的 LCD_IO 接口风格
 * @see 
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#ifndef _ST7789V_H_
#define _ST7789V_H_

#ifdef __cplusplus
 extern "C" {
#endif 

/************ 头文件包含 ***********/
#include <stdint.h>
#include <stdlib.h>

/**
 * @brief ST7789V 显示尺寸
 * 
 */
#define ST7789_LCD_WIDTH             240
#define ST7789_LCD_HEIGHT            280

/**
 * @brief 矩形填充发送模式。
 *
 * - ST7789_FILL_MODE_PIXEL： 每个像素单独发送 2 字节，逻辑最简单，但速度最慢。
 * - ST7789_FILL_MODE_BUFFER：先填充颜色缓冲区，再用 SPI 阻塞方式一次发送一块缓冲区。
 * - ST7789_FILL_MODE_DMA：   先填充颜色缓冲区，再用 SPI + DMA 发送一块缓冲区。
 */
#define ST7789_FILL_MODE_PIXEL       0U
#define ST7789_FILL_MODE_BUFFER      1U
#define ST7789_FILL_MODE_DMA         2U

#ifndef ST7789_FILL_MODE
#define ST7789_FILL_MODE             ST7789_FILL_MODE_DMA // SPI+DMA
#endif

/**
 * @brief 矩形填充缓冲区一次缓存的像素数量。
 *
 * 一个 RGB565 像素占 2 字节。默认 128 个像素，即 256 字节缓冲区。
 * ST7789_FILL_MODE_BUFFER 和 ST7789_FILL_MODE_DMA 会使用这个宏。
 */
#ifndef ST7789_FILL_BUFFER_PIXELS
#define ST7789_FILL_BUFFER_PIXELS    128U
#endif

#if (ST7789_FILL_BUFFER_PIXELS == 0U)
#error "ST7789_FILL_BUFFER_PIXELS must be greater than 0"
#endif

#if (ST7789_FILL_BUFFER_PIXELS > 32767U)
#error "ST7789_FILL_BUFFER_PIXELS is too large for one SPI transfer"
#endif

/**
 * @brief ST7789V 命令表 1
 * 
 */
#define ST7789_CMD_NOP               0x00  // 空操作
#define ST7789_CMD_SWRESET           0x01  // 软件复位
#define ST7789_CMD_RDDID             0x04  // 读取显示 ID
#define ST7789_CMD_RDDST             0x09  // 读取显示状态
#define ST7789_CMD_RDDPM             0x0a  // 读取显示电源模式
#define ST7789_CMD_RDDMADCTL         0x0b  // 读取显示方向控制
#define ST7789_CMD_RDDCOLMOD         0x0c  // 读取像素格式
#define ST7789_CMD_RDDIM             0x0d  // 读取显示图像模式
#define ST7789_CMD_RDDSM             0x0e  // 读取显示信号模式
#define ST7789_CMD_RDDSDR            0x0f  // 读取自检结果
#define ST7789_CMD_SLPIN             0x10  // 进入睡眠
#define ST7789_CMD_SLPOUT            0x11  // 退出睡眠
#define ST7789_CMD_PTLON             0x12  // 打开局部显示模式
#define ST7789_CMD_NORON             0x13  // 打开普通显示模式
#define ST7789_CMD_INVOFF            0x20  // 关闭显示反色
#define ST7789_CMD_INVON             0x21  // 打开显示反色
#define ST7789_CMD_GAMSET            0x26  // 设置 Gamma 曲线
#define ST7789_CMD_DISPOFF           0x28  // 关闭显示
#define ST7789_CMD_DISPON            0x29  // 打开显示
#define ST7789_CMD_CASET             0x2a  // 设置列地址
#define ST7789_CMD_RASET             0x2b  // 设置行地址
#define ST7789_CMD_RAMWR             0x2c  // 写显存
#define ST7789_CMD_RAMRD             0x2e  // 读显存
#define ST7789_CMD_PTLAR             0x30  // 设置局部显示起止地址
#define ST7789_CMD_VSCRDEF           0x33  // 设置垂直滚动区域
#define ST7789_CMD_TEOFF             0x34  // 关闭撕裂效果线
#define ST7789_CMD_TEON              0x35  // 打开撕裂效果线
#define ST7789_CMD_MADCTL            0x36  // 存储器访问控制
#define ST7789_CMD_VSCRSADD          0x37  // 设置垂直滚动起始地址
#define ST7789_CMD_IDMOFF            0x38  // 关闭空闲模式
#define ST7789_CMD_IDMON             0x39  // 打开空闲模式
#define ST7789_CMD_COLMOD            0x3a  // 设置接口像素格式
#define ST7789_CMD_RAMWRC            0x3c  // 继续写显存
#define ST7789_CMD_RAMRDC            0x3e  // 继续读显存
#define ST7789_CMD_TESCAN            0x44  // 设置撕裂效果扫描线
#define ST7789_CMD_RDTESCAN          0x45  // 读取扫描线
#define ST7789_CMD_WRDISBV           0x51  // 写显示亮度
#define ST7789_CMD_RDDISBV           0x52  // 读显示亮度
#define ST7789_CMD_WRCTRLD           0x53  // 写显示控制
#define ST7789_CMD_RDCTRLD           0x54  // 读显示控制
#define ST7789_CMD_WRCACE            0x55  // 写内容自适应亮度控制
#define ST7789_CMD_RDCABC            0x56  // 读内容自适应亮度控制
#define ST7789_CMD_WRCABCMB          0x5e  // 写 CABC 最小亮度
#define ST7789_CMD_RDCABCMB          0x5f  // 读 CABC 最小亮度
#define ST7789_CMD_RDABCSDR          0x68  // 读取自动亮度控制自检结果
#define ST7789_CMD_RDID1             0xda  // 读取 ID1
#define ST7789_CMD_RDID2             0xdb  // 读取 ID2
#define ST7789_CMD_RDID3             0xdc  // 读取 ID3

/**
 * @brief ST7789V 命令表 2
 * 
 */
#define ST7789_CMD_RAMCTRL           0xb0  // RAM 控制
#define ST7789_CMD_RGBCTRL           0xb1  // RGB 控制
#define ST7789_CMD_PORCTRL           0xb2  // Porch 控制
#define ST7789_CMD_FRCTRL1           0xb3  // 帧率控制 1
#define ST7789_CMD_GCTRL             0xb7  // 栅极控制
#define ST7789_CMD_DGMEN             0xba  // 数字 Gamma 使能
#define ST7789_CMD_VCOMS             0xbb  // VCOM 设置
#define ST7789_CMD_LCMCTRL           0xc0  // LCM 控制
#define ST7789_CMD_IDSET             0xc1  // ID 设置
#define ST7789_CMD_VDVVRHEN          0xc2  // VDV 和 VRH 命令使能
#define ST7789_CMD_VRHS              0xc3  // VRH 设置
#define ST7789_CMD_VDVSET            0xc4  // VDV 设置
#define ST7789_CMD_VCMOFSET          0xc5  // VCOM 偏移设置
#define ST7789_CMD_FRCTR2            0xc6  // 帧率控制 2
#define ST7789_CMD_CABCCTRL          0xc7  // CABC 控制
#define ST7789_CMD_REGSEL1           0xc8  // 寄存器值选择 1
#define ST7789_CMD_REGSEL2           0xca  // 寄存器值选择 2
#define ST7789_CMD_PWMFRSEL          0xcc  // PWM 频率选择
#define ST7789_CMD_PWCTRL1           0xd0  // 电源控制 1
#define ST7789_CMD_VAPVANEN          0xd2  // 使能 VAP/VAN 信号输出
#define ST7789_CMD_CMD2EN            0xdf  // 命令表 2 使能
#define ST7789_CMD_PVGAMCTRL         0xe0  // 正电压 Gamma 控制
#define ST7789_CMD_NVGAMCTRL         0xe1  // 负电压 Gamma 控制
#define ST7789_CMD_DGMLUTR           0xe2  // 红色数字 Gamma 查找表
#define ST7789_CMD_DGMLUTB           0xe3  // 蓝色数字 Gamma 查找表
#define ST7789_CMD_GATECTRL          0xe4  // 栅极控制
#define ST7789_CMD_PWCTRL2           0xe8  // 电源控制 2
#define ST7789_CMD_EQCTRL            0xe9  // 均衡时间控制
#define ST7789_CMD_PROMCTRL          0xec  // 程序控制
#define ST7789_CMD_PROMEN            0xfa  // 程序模式使能
#define ST7789_CMD_NVMSET            0xfc  // NVM 设置
#define ST7789_CMD_PROMACT           0xfe  // 程序动作

#define ST7789_CMDLIST_END           0xff  // 命令序列结束标记

/**** 常用 16 位颜色值 ***************/
#define	ST7789_BLACK   0x0000
#define	ST7789_BLUE    0x001F
#define	ST7789_RED     0xF800
#define	ST7789_GREEN   0x07E0
#define ST7789_CYAN    0x07FF
#define ST7789_MAGENTA 0xF81F
#define ST7789_YELLOW  0xFFE0
#define ST7789_WHITE   0xFFFF

/********** ST7789 LCD 驱动导出函数 ***********************/

/* LCD 驱动公开 API，应用层代码应调用这些函数。 */
void st7789_Init(void);
void st7789_DeInit(void);
void st7789_Reset(void);
void st7789_DisplayOn(void);
void st7789_DisplayOff(void);
void st7789_SetBacklight(uint8_t brightness);
void st7789_TxCpltCallback(void);

void st7789_SetWindow(uint16_t xStart, uint16_t yStart, uint16_t xEnd, uint16_t yEnd);
void st7789_WritePixels(const uint8_t *pixels, uint32_t length);
void st7789_FillArea(uint16_t color, uint16_t startX, uint16_t startY, uint16_t width, uint16_t height);
void st7789_Clear(uint16_t color);

#ifdef __cplusplus
 }
#endif 

#endif  // _ST7789V_H_
