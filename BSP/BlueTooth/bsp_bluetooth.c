#include "bsp_bluetooth.h"

#include "FreeRTOS.h"
#include "semphr.h"
#include "main.h"

#define BSP_BLUETOOTH_EN_GPIO_PORT       GPIOA
#define BSP_BLUETOOTH_EN_GPIO_PIN        GPIO_PIN_8

#define BSP_BLUETOOTH_UART_TX_GPIO_PORT  GPIOA
#define BSP_BLUETOOTH_UART_TX_GPIO_PIN   GPIO_PIN_9
#define BSP_BLUETOOTH_UART_RX_GPIO_PORT  GPIOA
#define BSP_BLUETOOTH_UART_RX_GPIO_PIN   GPIO_PIN_10
#define BSP_BLUETOOTH_UART_AF            GPIO_AF7_USART1

#define BSP_BLUETOOTH_UART               USART1
#define BSP_BLUETOOTH_UART_BAUDRATE      115200U
#define BSP_BLUETOOTH_UART_IRQn          USART1_IRQn
#define BSP_BLUETOOTH_TX_DMA_STREAM      DMA2_Stream7
#define BSP_BLUETOOTH_TX_DMA_CHANNEL     DMA_CHANNEL_4
#define BSP_BLUETOOTH_TX_DMA_IRQn        DMA2_Stream7_IRQn
#define BSP_BLUETOOTH_LOCK_TIMEOUT_MS    200U

static UART_HandleTypeDef bluetooth_uart_handle;
static DMA_HandleTypeDef bluetooth_uart_tx_dma_handle;
static StaticSemaphore_t bluetooth_uart_lock_storage;
static SemaphoreHandle_t bluetooth_uart_lock;
static BSP_BlueTooth_IsrHook_t bluetooth_tx_complete_hook;
static void * bluetooth_tx_complete_hook_context;
static BSP_BlueTooth_IsrHook_t bluetooth_error_hook;
static void * bluetooth_error_hook_context;
static uint8_t bluetooth_gpio_initialized;
static uint8_t bluetooth_uart_initialized;
static uint8_t bluetooth_dma_initialized;
static uint8_t bluetooth_uart_lock_initialized;
static volatile uint8_t bluetooth_tx_busy;
static volatile uint8_t bluetooth_tx_done;
static volatile uint8_t bluetooth_tx_error;

static void BSP_BlueTooth_GPIO_Init(void);
static int BSP_BlueTooth_UART_Init(void);
static int BSP_BlueTooth_DMA_Init(void);
static void BSP_BlueTooth_LockInit(void);
static int BSP_BlueTooth_Lock(uint32_t timeout_ms);
static void BSP_BlueTooth_Unlock(void);
static void BSP_BlueTooth_UnlockFromISR(BaseType_t * higher_priority_task_woken);

/**
 * @brief 初始化蓝牙模块 GPIO 和 USART1。
 */
void BSP_BlueTooth_Init(void)
{
    BSP_BlueTooth_GPIO_Init();
    BSP_BlueTooth_LockInit();
    (void)BSP_BlueTooth_UART_Init();

    // 蓝牙 EN 为高电平有效，初始化完成后默认打开模块，便于后续串口通信。
    BSP_BlueTooth_Enable();
}

/**
 * @brief 关闭蓝牙模块并反初始化 USART1、DMA 和相关 GPIO。
 */
void BSP_BlueTooth_DeInit(void)
{
    BSP_BlueTooth_Disable();

    if(bluetooth_uart_initialized != 0U) {
        (void)HAL_UART_DeInit(&bluetooth_uart_handle);
        HAL_NVIC_DisableIRQ(BSP_BLUETOOTH_UART_IRQn);
        __HAL_RCC_USART1_CLK_DISABLE();
        bluetooth_uart_initialized = 0U;
    }

    if(bluetooth_dma_initialized != 0U) {
        (void)HAL_DMA_DeInit(&bluetooth_uart_tx_dma_handle);
        HAL_NVIC_DisableIRQ(BSP_BLUETOOTH_TX_DMA_IRQn);
        bluetooth_dma_initialized = 0U;
    }

    HAL_GPIO_DeInit(GPIOA,
                    BSP_BLUETOOTH_EN_GPIO_PIN |
                    BSP_BLUETOOTH_UART_TX_GPIO_PIN |
                    BSP_BLUETOOTH_UART_RX_GPIO_PIN);

    bluetooth_gpio_initialized = 0U;
    bluetooth_tx_busy = 0U;
    bluetooth_tx_done = 0U;
    bluetooth_tx_error = 0U;
    BSP_BlueTooth_Unlock();
}

/**
 * @brief 打开蓝牙模块电源使能脚。
 */
void BSP_BlueTooth_Enable(void)
{
    BSP_BlueTooth_GPIO_Init();

    HAL_GPIO_WritePin(BSP_BLUETOOTH_EN_GPIO_PORT,
                      BSP_BLUETOOTH_EN_GPIO_PIN,
                      GPIO_PIN_SET);
}

/**
 * @brief 关闭蓝牙模块电源使能脚。
 */
void BSP_BlueTooth_Disable(void)
{
    BSP_BlueTooth_GPIO_Init();

    HAL_GPIO_WritePin(BSP_BLUETOOTH_EN_GPIO_PORT,
                      BSP_BLUETOOTH_EN_GPIO_PIN,
                      GPIO_PIN_RESET);
}

/**
 * @brief 查询蓝牙模块当前使能状态。
 */
uint8_t BSP_BlueTooth_IsEnabled(void)
{
    BSP_BlueTooth_GPIO_Init();

    return (HAL_GPIO_ReadPin(BSP_BLUETOOTH_EN_GPIO_PORT,
                             BSP_BLUETOOTH_EN_GPIO_PIN) == GPIO_PIN_SET) ? 1U : 0U;
}

/**
 * @brief 通过 USART1 阻塞发送数据到蓝牙模块。
 *
 * 这里使用同一把串口资源锁保护阻塞发送、DMA 发送和阻塞接收，避免多个任务同时操作
 * 同一个 HAL UART 状态机，导致发送被打断或返回忙错误。
 */
int BSP_BlueTooth_Send(const uint8_t * data, uint16_t len, uint32_t timeout_ms)
{
    if((data == 0) && (len != 0U)) {
        return -1;
    }

    if(BSP_BlueTooth_UART_Init() != 0) {
        return -2;
    }

    if(len == 0U) {
        return 0;
    }

    if(BSP_BlueTooth_Lock(timeout_ms) != 0) {
        return -3;
    }

    bluetooth_tx_busy = 1U;
    bluetooth_tx_done = 0U;
    bluetooth_tx_error = 0U;

    if(HAL_UART_Transmit(&bluetooth_uart_handle, (uint8_t *)data, len, timeout_ms) != HAL_OK) {
        bluetooth_tx_busy = 0U;
        bluetooth_tx_error = 1U;
        BSP_BlueTooth_Unlock();
        return -3;
    }

    bluetooth_tx_busy = 0U;
    BSP_BlueTooth_Unlock();
    return 0;
}

/**
 * @brief 通过 USART1 TX DMA 非阻塞发送数据到蓝牙模块。
 *
 * DMA 启动前先拿串口资源锁；锁会一直持有到 DMA 完成中断或错误中断里释放。
 * 这样即使别的任务随后调用阻塞发送，也只能等这次 DMA 真正完成后再继续。
 */
int BSP_BlueTooth_SendDma(const uint8_t * data, uint16_t len)
{
    if((data == 0) && (len != 0U)) {
        return -1;
    }

    if(BSP_BlueTooth_UART_Init() != 0) {
        return -2;
    }

    if(len == 0U) {
        return 0;
    }

    if(BSP_BlueTooth_Lock(BSP_BLUETOOTH_LOCK_TIMEOUT_MS) != 0) {
        return -3;
    }

    bluetooth_tx_busy = 1U;
    bluetooth_tx_done = 0U;
    bluetooth_tx_error = 0U;

    // DMA 只保存源缓冲区地址，不复制数据；发送完成前 data 指向的内存必须保持有效。
    if(HAL_UART_Transmit_DMA(&bluetooth_uart_handle, (uint8_t *)data, len) != HAL_OK) {
        bluetooth_tx_busy = 0U;
        bluetooth_tx_error = 1U;
        BSP_BlueTooth_Unlock();
        return -4;
    }

    return 0;
}

/**
 * @brief 查询 USART1 TX 是否仍在发送。
 */
uint8_t BSP_BlueTooth_IsTxBusy(void)
{
    return bluetooth_tx_busy;
}

/**
 * @brief 读取并清除最近一次 USART1 TX DMA 传输完成标志。
 */
uint8_t BSP_BlueTooth_TakeTxDone(void)
{
    uint8_t done = bluetooth_tx_done;

    bluetooth_tx_done = 0U;
    return done;
}

/**
 * @brief USART1 TX DMA 中断分发入口。
 */
void BSP_BlueTooth_DMA_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&bluetooth_uart_tx_dma_handle);
}

/**
 * @brief USART1 全局中断分发入口。
 */
void BSP_BlueTooth_UART_IRQHandler(void)
{
    HAL_UART_IRQHandler(&bluetooth_uart_handle);
}

UART_HandleTypeDef * BSP_BlueTooth_GetUartHandle(void)
{
    (void)BSP_BlueTooth_UART_Init();
    return &bluetooth_uart_handle;
}

void BSP_BlueTooth_RegisterTxCompleteHook(BSP_BlueTooth_IsrHook_t hook, void * context)
{
    bluetooth_tx_complete_hook = hook;
    bluetooth_tx_complete_hook_context = context;
}

void BSP_BlueTooth_RegisterErrorHook(BSP_BlueTooth_IsrHook_t hook, void * context)
{
    bluetooth_error_hook = hook;
    bluetooth_error_hook_context = context;
}

/**
 * @brief 通过 USART1 阻塞接收蓝牙模块数据。
 */
int BSP_BlueTooth_Receive(uint8_t * data, uint16_t len, uint32_t timeout_ms)
{
    if((data == 0) && (len != 0U)) {
        return -1;
    }

    if(BSP_BlueTooth_UART_Init() != 0) {
        return -2;
    }

    if(len == 0U) {
        return 0;
    }

    if(BSP_BlueTooth_Lock(timeout_ms) != 0) {
        return -3;
    }

    if(HAL_UART_Receive(&bluetooth_uart_handle, data, len, timeout_ms) != HAL_OK) {
        BSP_BlueTooth_Unlock();
        return -3;
    }

    BSP_BlueTooth_Unlock();
    return 0;
}

/**
 * @brief 初始化 PA8 EN 和 PA9/PA10 USART1 复用引脚。
 */
static void BSP_BlueTooth_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    if(bluetooth_gpio_initialized != 0U) {
        return;
    }

    __HAL_RCC_GPIOA_CLK_ENABLE();

    HAL_GPIO_WritePin(BSP_BLUETOOTH_EN_GPIO_PORT,
                      BSP_BLUETOOTH_EN_GPIO_PIN,
                      GPIO_PIN_RESET);

    GPIO_InitStruct.Pin = BSP_BLUETOOTH_EN_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(BSP_BLUETOOTH_EN_GPIO_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = BSP_BLUETOOTH_UART_TX_GPIO_PIN | BSP_BLUETOOTH_UART_RX_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = BSP_BLUETOOTH_UART_AF;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    bluetooth_gpio_initialized = 1U;
}

/**
 * @brief 初始化 USART1_TX 对应的 DMA2 Stream7 Channel4。
 */
static int BSP_BlueTooth_DMA_Init(void)
{
    if(bluetooth_dma_initialized != 0U) {
        return 0;
    }

    __HAL_RCC_DMA2_CLK_ENABLE();

    bluetooth_uart_tx_dma_handle.Instance = BSP_BLUETOOTH_TX_DMA_STREAM;
    bluetooth_uart_tx_dma_handle.Init.Channel = BSP_BLUETOOTH_TX_DMA_CHANNEL;
    bluetooth_uart_tx_dma_handle.Init.Direction = DMA_MEMORY_TO_PERIPH;
    bluetooth_uart_tx_dma_handle.Init.PeriphInc = DMA_PINC_DISABLE;
    bluetooth_uart_tx_dma_handle.Init.MemInc = DMA_MINC_ENABLE;
    bluetooth_uart_tx_dma_handle.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    bluetooth_uart_tx_dma_handle.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    bluetooth_uart_tx_dma_handle.Init.Mode = DMA_NORMAL;
    bluetooth_uart_tx_dma_handle.Init.Priority = DMA_PRIORITY_LOW;
    bluetooth_uart_tx_dma_handle.Init.FIFOMode = DMA_FIFOMODE_DISABLE;

    if(HAL_DMA_Init(&bluetooth_uart_tx_dma_handle) != HAL_OK) {
        return -1;
    }

    __HAL_LINKDMA(&bluetooth_uart_handle, hdmatx, bluetooth_uart_tx_dma_handle);

    HAL_NVIC_SetPriority(BSP_BLUETOOTH_TX_DMA_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(BSP_BLUETOOTH_TX_DMA_IRQn);

    bluetooth_dma_initialized = 1U;
    return 0;
}

/**
 * @brief 初始化 USART1 为 115200-8-N-1 收发模式。
 */
static int BSP_BlueTooth_UART_Init(void)
{
    if(bluetooth_uart_initialized != 0U) {
        return 0;
    }

    BSP_BlueTooth_GPIO_Init();
    BSP_BlueTooth_LockInit();
    __HAL_RCC_USART1_CLK_ENABLE();

    bluetooth_uart_handle.Instance = BSP_BLUETOOTH_UART;
    bluetooth_uart_handle.Init.BaudRate = BSP_BLUETOOTH_UART_BAUDRATE;
    bluetooth_uart_handle.Init.WordLength = UART_WORDLENGTH_8B;
    bluetooth_uart_handle.Init.StopBits = UART_STOPBITS_1;
    bluetooth_uart_handle.Init.Parity = UART_PARITY_NONE;
    bluetooth_uart_handle.Init.Mode = UART_MODE_TX_RX;
    bluetooth_uart_handle.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    bluetooth_uart_handle.Init.OverSampling = UART_OVERSAMPLING_16;

    if(BSP_BlueTooth_DMA_Init() != 0) {
        return -1;
    }

    if(HAL_UART_Init(&bluetooth_uart_handle) != HAL_OK) {
        return -1;
    }

    // DMA Normal 模式下 HAL 会在 DMA 完成后等待 USART TC 中断，再调用 TxCpltCallback。
    HAL_NVIC_SetPriority(BSP_BLUETOOTH_UART_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(BSP_BLUETOOTH_UART_IRQn);

    bluetooth_uart_initialized = 1U;
    return 0;
}

/**
 * @brief 初始化蓝牙串口资源锁。
 */
static void BSP_BlueTooth_LockInit(void)
{
    if(bluetooth_uart_lock_initialized != 0U) {
        return;
    }

    bluetooth_uart_lock = xSemaphoreCreateBinaryStatic(&bluetooth_uart_lock_storage);
    if(bluetooth_uart_lock != NULL) {
        (void)xSemaphoreGive(bluetooth_uart_lock);
        bluetooth_uart_lock_initialized = 1U;
    }
}

/**
 * @brief 获取蓝牙串口资源锁。
 */
static int BSP_BlueTooth_Lock(uint32_t timeout_ms)
{
    TickType_t timeout_ticks;

    BSP_BlueTooth_LockInit();
    if(bluetooth_uart_lock == NULL) {
        return -1;
    }

    if(timeout_ms == HAL_MAX_DELAY) {
        timeout_ticks = portMAX_DELAY;
    } else {
        timeout_ticks = pdMS_TO_TICKS(timeout_ms);
        if((timeout_ms > 0U) && (timeout_ticks == 0U)) {
            timeout_ticks = 1U;
        }
    }

    return (xSemaphoreTake(bluetooth_uart_lock, timeout_ticks) == pdTRUE) ? 0 : -1;
}

/**
 * @brief 在任务上下文里释放蓝牙串口资源锁。
 */
static void BSP_BlueTooth_Unlock(void)
{
    if(bluetooth_uart_lock != NULL) {
        (void)xSemaphoreGive(bluetooth_uart_lock);
    }
}

/**
 * @brief 在中断上下文里释放蓝牙串口资源锁。
 */
static void BSP_BlueTooth_UnlockFromISR(BaseType_t * higher_priority_task_woken)
{
    if(bluetooth_uart_lock != NULL) {
        (void)xSemaphoreGiveFromISR(bluetooth_uart_lock, higher_priority_task_woken);
    }
}

/**
 * @brief HAL UART 发送完成回调。
 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if(huart == &bluetooth_uart_handle) {
        BaseType_t higher_priority_task_woken = pdFALSE;

        bluetooth_tx_busy = 0U;
        bluetooth_tx_done = 1U;
        BSP_BlueTooth_UnlockFromISR(&higher_priority_task_woken);
        if(bluetooth_tx_complete_hook != NULL) {
            bluetooth_tx_complete_hook(bluetooth_tx_complete_hook_context);
        }
        portYIELD_FROM_ISR(higher_priority_task_woken);
    }
}

/**
 * @brief HAL UART 错误回调。
 */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if(huart == &bluetooth_uart_handle) {
        BaseType_t higher_priority_task_woken = pdFALSE;

        bluetooth_tx_busy = 0U;
        bluetooth_tx_error = 1U;
        BSP_BlueTooth_UnlockFromISR(&higher_priority_task_woken);
        if(bluetooth_error_hook != NULL) {
            bluetooth_error_hook(bluetooth_error_hook_context);
        }
        portYIELD_FROM_ISR(higher_priority_task_woken);
    }
}
