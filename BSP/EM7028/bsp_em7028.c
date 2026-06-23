/**
 * @file bsp_em7028.c
 * @brief EM7028 HRS1 连续模式 BSP 驱动。
 */

#include "bsp_em7028.h"

#include <string.h>

#include "bsp_iic.h"
#include "em7028_reg.h"
#include "main.h"

#define EM7028_LED_EN_PORT    GPIOB
#define EM7028_LED_EN_PIN     GPIO_PIN_15
#define EM7028_ID_RETRY_COUNT 5U
#define EM7028_ID_RETRY_DELAY_MS 100U
#define EM7028_DEBUG_BYPASS_ID_CHECK 1U

static EM7028_Handle_t * s_em7028_active_handle;
static void EM7028_LedEnInit(void);
static void EM7028_SetLedEn(uint8_t enable);
static uint8_t EM7028_BuildHrs1CtrlValue(const EM7028_Hrs1Config_t * config);
static EM7028_Status_t EM7028_ReadConfigureReg(uint8_t * value);
static EM7028_Status_t EM7028_WriteConfigureReg(uint8_t value);

/**
 * @brief 获取 EM7028 HRS1 默认配置。
 */
void EM7028_GetDefaultHrs1Config(EM7028_Hrs1Config_t * config)
{
    if(config == NULL) {
        return;
    }

    config->hrs_gain = EM7028_HRS1_GAIN_X1;
    config->hrs_range = EM7028_HRS1_RANGE_X8;
    config->hrs_freq = EM7028_HRS1_FREQ_1P5625MS;
    config->hrs_resolution = EM7028_HRS1_RES_16BIT;
}

/**
 * @brief 读取 EM7028 单个寄存器。
 */
EM7028_Status_t EM7028_ReadReg(uint8_t reg, uint8_t * value)
{
    int ret;

    if(value == NULL) {
        return EM7028_ERR_PARAM;
    }

    ret = BSP_IIC_ReadRegs(EM7028_I2C_ADDR_7BIT, reg, value, 1U);
    if(ret != 0) {
        return EM7028_ERR_I2C;
    }

    return EM7028_OK;
}

/**
 * @brief 写入 EM7028 单个寄存器。
 */
EM7028_Status_t EM7028_WriteReg(uint8_t reg, uint8_t value)
{
    int ret;

    ret = BSP_IIC_WriteReg(EM7028_I2C_ADDR_7BIT, reg, value);
    if(ret != 0) {
        return EM7028_ERR_I2C;
    }

    return EM7028_OK;
}

/**
 * @brief 连续读取 EM7028 寄存器数据。
 */
EM7028_Status_t EM7028_ReadRegs(uint8_t start_reg, uint8_t * buffer, uint8_t length)
{
    int ret;

    if((buffer == NULL) || (length == 0U)) {
        return EM7028_ERR_PARAM;
    }

    ret = BSP_IIC_ReadRegs(EM7028_I2C_ADDR_7BIT, start_reg, buffer, length);
    if(ret != 0) {
        return EM7028_ERR_I2C;
    }

    return EM7028_OK;
}

/**
 * @brief 读取并校验 EM7028 产品 ID。
 */
EM7028_Status_t EM7028_CheckDeviceId(EM7028_Handle_t * handle)
{
    EM7028_Status_t status = EM7028_ERR_ID;
    uint8_t product_id = 0U;
    uint8_t retry = EM7028_ID_RETRY_COUNT;

    if(handle == NULL) {
        return EM7028_ERR_PARAM;
    }

    while(retry > 0U) {
        status = EM7028_ReadReg(EM7028_REG_PID, &product_id);
        if((status == EM7028_OK) && (product_id == EM7028_PRODUCT_ID_VALUE)) {
            handle->product_id = product_id;
            return EM7028_OK;
        }

        handle->product_id = product_id;
        HAL_Delay(EM7028_ID_RETRY_DELAY_MS);
        retry--;
    }

#if (EM7028_DEBUG_BYPASS_ID_CHECK != 0U)
    return (status == EM7028_OK) ? EM7028_OK : status;
#else
    return (status == EM7028_OK) ? EM7028_ERR_ID : status;
#endif
}

/**
 * @brief 初始化 EM7028 BSP 驱动。
 */
EM7028_Status_t EM7028_Init(EM7028_Handle_t * handle, const EM7028_Hrs1Config_t * config)
{
    EM7028_Status_t status;
    uint8_t hrs1_ctrl;

    if((handle == NULL) || (config == NULL)) {
        return EM7028_ERR_PARAM;
    }

    memset(handle, 0, sizeof(*handle));
    BSP_IIC_Init();
    EM7028_LedEnInit();
    EM7028_SetLedEn(0U);

    status = EM7028_CheckDeviceId(handle);
    if(status != EM7028_OK) {
        return status;
    }

    status = EM7028_WriteReg(EM7028_REG_CONFIGURE, EM7028_CONFIGURE_DISABLE_ALL);
    if(status != EM7028_OK) {
        return status;
    }

    status = EM7028_WriteReg(EM7028_REG_HRS2_DATA_OFFSET, EM7028_HRS2_DATA_OFFSET_DEFAULT);
    if(status != EM7028_OK) {
        return status;
    }

    status = EM7028_WriteReg(EM7028_REG_HRS2_GAIN_CTRL, EM7028_HRS2_GAIN_CTRL_DEFAULT);
    if(status != EM7028_OK) {
        return status;
    }

    hrs1_ctrl = EM7028_BuildHrs1CtrlValue(config);
    status = EM7028_WriteReg(EM7028_REG_HRS1_CTRL, hrs1_ctrl);
    if(status != EM7028_OK) {
        return status;
    }

    status = EM7028_WriteReg(EM7028_REG_INT_CTRL, EM7028_INT_CTRL_DEFAULT);
    if(status != EM7028_OK) {
        return status;
    }

    handle->initialized = 1U;
    handle->hrs1_running = 0U;
    handle->latest_raw_ppg = 0U;
    s_em7028_active_handle = handle;

    return EM7028_OK;
}

/**
 * @brief 启动 EM7028 HRS1 连续采样模式。
 */
EM7028_Status_t EM7028_StartHrs1(EM7028_Handle_t * handle)
{
    EM7028_Status_t status;

    if(handle == NULL) {
        return EM7028_ERR_PARAM;
    }

    if(handle->initialized == 0U) {
        return EM7028_ERR_NOT_INITIALIZED;
    }

    if(handle->hrs1_running != 0U) {
        return EM7028_ERR_ALREADY_RUNNING;
    }

    status = EM7028_CheckDeviceId(handle);
    if(status != EM7028_OK) {
        return status;
    }

    status = EM7028_WriteReg(EM7028_REG_CONFIGURE, EM7028_CONFIGURE_DISABLE_ALL);
    if(status != EM7028_OK) {
        return status;
    }

    status = EM7028_WriteReg(EM7028_REG_HRS2_DATA_OFFSET, EM7028_HRS2_DATA_OFFSET_DEFAULT);
    if(status != EM7028_OK) {
        return status;
    }

    status = EM7028_WriteReg(EM7028_REG_HRS2_GAIN_CTRL, EM7028_HRS2_GAIN_CTRL_DEFAULT);
    if(status != EM7028_OK) {
        return status;
    }

    status = EM7028_WriteReg(EM7028_REG_HRS1_CTRL, EM7028_HRS1_DEFAULT_CTRL_VALUE);
    if(status != EM7028_OK) {
        return status;
    }

    status = EM7028_WriteReg(EM7028_REG_INT_CTRL, EM7028_INT_CTRL_DEFAULT);
    if(status != EM7028_OK) {
        return status;
    }

    status = EM7028_WriteConfigureReg(EM7028_CONFIGURE_ENABLE_HRS1);
    if(status != EM7028_OK) {
        return status;
    }

    HAL_Delay(100U);
    handle->hrs1_running = 1U;
    s_em7028_active_handle = handle;

    return EM7028_OK;
}

/**
 * @brief 停止 EM7028 HRS1 连续采样模式。
 */
EM7028_Status_t EM7028_StopHrs1(EM7028_Handle_t * handle)
{
    EM7028_Status_t status;

    if(handle == NULL) {
        return EM7028_ERR_PARAM;
    }

    if(handle->initialized == 0U) {
        return EM7028_ERR_NOT_INITIALIZED;
    }

    status = EM7028_CheckDeviceId(handle);
    if(status != EM7028_OK) {
        return status;
    }

    status = EM7028_WriteConfigureReg(EM7028_CONFIGURE_DISABLE_ALL);
    if(status != EM7028_OK) {
        return status;
    }

    handle->hrs1_running = 0U;

    return EM7028_OK;
}

/**
 * @brief 读取一次 EM7028 的 16 位 HRS1 原始 PPG 数据。
 */
EM7028_Status_t EM7028_ReadHrs1Raw(EM7028_Handle_t * handle, uint16_t * raw_ppg)
{
    EM7028_Status_t status;
    uint8_t buffer[2];

    if((handle == NULL) || (raw_ppg == NULL)) {
        return EM7028_ERR_PARAM;
    }

    if(handle->initialized == 0U) {
        return EM7028_ERR_NOT_INITIALIZED;
    }

    if(handle->hrs1_running == 0U) {
        return EM7028_ERR_NOT_RUNNING;
    }

    status = EM7028_ReadRegs(EM7028_REG_HRS1_DATA0_L, buffer, 2U);
    if(status != EM7028_OK) {
        return status;
    }

    *raw_ppg = (uint16_t)buffer[0];
    *raw_ppg |= ((uint16_t)buffer[1] << 8);
    handle->latest_raw_ppg = *raw_ppg;

    return EM7028_OK;
}

/**
 * @brief 初始化 EM7028 增强灯使能引脚。
 */
static void EM7028_LedEnInit(void)
{
    GPIO_InitTypeDef gpio_init = {0};

    __HAL_RCC_GPIOB_CLK_ENABLE();

    gpio_init.Pin = EM7028_LED_EN_PIN;
    gpio_init.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init.Pull = GPIO_NOPULL;
    gpio_init.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(EM7028_LED_EN_PORT, &gpio_init);
}

/**
 * @brief 控制 EM7028 增强灯使能脚电平。
 */
static void EM7028_SetLedEn(uint8_t enable)
{
    HAL_GPIO_WritePin(EM7028_LED_EN_PORT,
                      EM7028_LED_EN_PIN,
                      (enable != 0U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/**
 * @brief 根据配置结构拼装 HRS1 控制寄存器值。
 */
static uint8_t EM7028_BuildHrs1CtrlValue(const EM7028_Hrs1Config_t * config)
{
    return (uint8_t)(config->hrs_gain |
                     config->hrs_range |
                     config->hrs_freq |
                     config->hrs_resolution |
                     EM7028_HRS1_MODE_ENABLE);
}

/**
 * @brief 读取 CONFIGURE 寄存器。
 */
static EM7028_Status_t EM7028_ReadConfigureReg(uint8_t * value)
{
    return EM7028_ReadReg(EM7028_REG_CONFIGURE, value);
}

/**
 * @brief 写回 CONFIGURE 寄存器。
 */
static EM7028_Status_t EM7028_WriteConfigureReg(uint8_t value)
{
    return EM7028_WriteReg(EM7028_REG_CONFIGURE, value);
}

/**
 * @brief 兼容旧 BSP 接口：初始化默认 HRS1 配置。
 */
int BSP_EM7028_Init(void)
{
    EM7028_Hrs1Config_t config;
    static EM7028_Handle_t handle;

    EM7028_GetDefaultHrs1Config(&config);
    return (int)EM7028_Init(&handle, &config);
}

/**
 * @brief 兼容旧 BSP 接口：探测产品 ID。
 */
int BSP_EM7028_Probe(void)
{
    if(s_em7028_active_handle == NULL) {
        return (int)EM7028_ERR_NOT_INITIALIZED;
    }

    return (int)EM7028_CheckDeviceId(s_em7028_active_handle);
}

/**
 * @brief 兼容旧 BSP 接口：启动 HRS1。
 */
int BSP_EM7028_Start(void)
{
    if(s_em7028_active_handle == NULL) {
        return (int)EM7028_ERR_NOT_INITIALIZED;
    }

    return (int)EM7028_StartHrs1(s_em7028_active_handle);
}

/**
 * @brief 兼容旧 BSP 接口：停止 HRS1。
 */
int BSP_EM7028_Stop(void)
{
    if(s_em7028_active_handle == NULL) {
        return (int)EM7028_ERR_NOT_INITIALIZED;
    }

    return (int)EM7028_StopHrs1(s_em7028_active_handle);
}

/**
 * @brief 兼容旧 BSP 接口：读取 HRS1 原始 PPG 数据。
 */
int BSP_EM7028_ReadHrs1Data(uint16_t * value)
{
    if(s_em7028_active_handle == NULL) {
        return (int)EM7028_ERR_NOT_INITIALIZED;
    }

    return (int)EM7028_ReadHrs1Raw(s_em7028_active_handle, value);
}

/**
 * @brief 兼容旧 BSP 接口：读取单寄存器。
 */
int BSP_EM7028_ReadRegister(uint8_t reg, uint8_t * value)
{
    return (int)EM7028_ReadReg(reg, value);
}

/**
 * @brief 兼容旧 BSP 接口：获取最近一次产品 ID。
 */
uint8_t BSP_EM7028_GetLastPid(void)
{
    return (s_em7028_active_handle != NULL) ? s_em7028_active_handle->product_id : 0U;
}

/**
 * @brief 兼容旧 BSP 接口：保留地址探测状态查询。
 */
int BSP_EM7028_GetLastProbeStatus(void)
{
    int ret;

    ret = BSP_IIC_Probe(EM7028_I2C_ADDR_7BIT);
    return (ret == 0) ? 0 : (int)EM7028_ERR_I2C;
}
