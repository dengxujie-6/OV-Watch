#ifndef BSP_BLUETOOTH_H
#define BSP_BLUETOOTH_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct __UART_HandleTypeDef UART_HandleTypeDef;
typedef void (*BSP_BlueTooth_IsrHook_t)(void * context);

/**
 * @brief 初始化蓝牙模块 GPIO 和 USART1。
 *
 * PA8 为 BlueTooth_EN，高电平有效；PA9/PA10 分别复用为 USART1_TX/RX。
 * BSP 层持有具体 GPIO、复用功能和 HAL UART 句柄，上层只能通过 HwAccess 调用。
 */
void BSP_BlueTooth_Init(void);

/**
 * @brief 关闭蓝牙模块并反初始化 USART1、DMA 和相关 GPIO。
 *
 * 该接口用于系统进入低功耗前收拢蓝牙相关资源；恢复时可再次调用
 * BSP_BlueTooth_Init() 重新建立串口和 DMA 状态。
 */
void BSP_BlueTooth_DeInit(void);

/**
 * @brief 打开蓝牙模块电源使能脚。
 */
void BSP_BlueTooth_Enable(void);

/**
 * @brief 关闭蓝牙模块电源使能脚。
 */
void BSP_BlueTooth_Disable(void);

/**
 * @brief 查询蓝牙模块当前使能状态。
 *
 * @return 1 表示 PA8 当前为高电平，0 表示低电平。
 */
uint8_t BSP_BlueTooth_IsEnabled(void);

/**
 * @brief 通过 USART1 阻塞发送数据到蓝牙模块。
 *
 * @param data 指向待发送缓冲区，不允许为 NULL，缓冲区由调用者持有。
 * @param len 待发送字节数，允许为 0。
 * @param timeout_ms HAL UART 阻塞发送超时时间，单位 ms。
 * @return 0 表示发送成功，负数表示参数错误或 HAL 发送失败。
 */
int BSP_BlueTooth_Send(const uint8_t * data, uint16_t len, uint32_t timeout_ms);

/**
 * @brief 通过 USART1 TX DMA 非阻塞发送数据到蓝牙模块。
 *
 * 该函数只启动一次 DMA 传输，实际发送完成由 DMA2_Stream7_IRQHandler()
 * 进入 HAL DMA 中断，再回调 HAL_UART_TxCpltCallback() 标记完成。
 *
 * @param data 指向待发送缓冲区，不允许为 NULL，DMA 完成前调用者必须保证缓冲区有效。
 * @param len 待发送字节数，允许为 0。
 * @return 0 表示 DMA 已启动或 len 为 0；负数表示参数错误、初始化失败、忙或 HAL 启动失败。
 */
int BSP_BlueTooth_SendDma(const uint8_t * data, uint16_t len);

/**
 * @brief 查询 USART1 TX DMA 是否仍在发送。
 * @return 1 表示发送中，0 表示空闲。
 */
uint8_t BSP_BlueTooth_IsTxBusy(void);

/**
 * @brief 读取并清除最近一次 USART1 TX DMA 传输完成标志。
 * @return 1 表示自上次读取后发生过一次完成中断，0 表示没有新的完成事件。
 */
uint8_t BSP_BlueTooth_TakeTxDone(void);

/**
 * @brief USART1 TX DMA 中断分发入口。
 */
void BSP_BlueTooth_DMA_IRQHandler(void);

/**
 * @brief USART1 全局中断分发入口。
 *
 * DMA Normal 模式下，DMA 完成后 HAL 会开启 USART TC 中断；
 * Core/Src/stm32f4xx_it.c 的 USART1_IRQHandler() 调用本函数，
 * 由 HAL_UART_IRQHandler() 完成最后一位移出后的发送完成回调。
 */
void BSP_BlueTooth_UART_IRQHandler(void);

/**
 * @brief 通过 USART1 阻塞接收蓝牙模块数据。
 *
 * @param data 指向接收缓冲区，不允许为 NULL，缓冲区由调用者持有。
 * @param len 期望接收字节数，允许为 0。
 * @param timeout_ms HAL UART 阻塞接收超时时间，单位 ms。
 * @return 0 表示接收成功，负数表示参数错误或 HAL 接收失败。
 */
int BSP_BlueTooth_Receive(uint8_t * data, uint16_t len, uint32_t timeout_ms);

UART_HandleTypeDef * BSP_BlueTooth_GetUartHandle(void);
void BSP_BlueTooth_RegisterTxCompleteHook(BSP_BlueTooth_IsrHook_t hook, void * context);
void BSP_BlueTooth_RegisterErrorHook(BSP_BlueTooth_IsrHook_t hook, void * context);

#ifdef __cplusplus
}
#endif

#endif /* BSP_BLUETOOTH_H */
