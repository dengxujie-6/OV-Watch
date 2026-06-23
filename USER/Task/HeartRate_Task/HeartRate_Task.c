#include "HeartRate_Task.h"

#include <stdio.h>

#include "cmsis_os2.h"
#include "hwaccess.h"

#define HEART_RATE_TASK_STARTUP_DELAY_MS   500U
#define HEART_RATE_TASK_SAMPLE_PERIOD_MS   100U
#define HEART_RATE_TASK_UART_TIMEOUT_MS    100U

static void HeartRate_Task_Log(const char * text);
static void HeartRate_Task_DumpRegisters(void);

/**
 * @brief 通过现有蓝牙串口发送一条调试文本。
 *
 * @param text 以 '\0' 结尾的文本，允许为 NULL。
 */
static void HeartRate_Task_Log(const char * text)
{
    uint16_t len = 0U;

    if((text == NULL) || (HwAccess.bluetooth.send == NULL)) {
        return;
    }

    while(text[len] != '\0') {
        len++;
    }

    if(len > 0U) {
        (void)HwAccess.bluetooth.send((const uint8_t *)text,
                                      len,
                                      HEART_RATE_TASK_UART_TIMEOUT_MS);
    }
}

/**
 * @brief 回读并打印当前 EM7028 关键寄存器，便于确认驱动配置是否生效。
 */
static void HeartRate_Task_DumpRegisters(void)
{
    char log_buf[80];
    uint8_t reg01 = 0U;
    uint8_t reg0d = 0U;
    uint8_t reg28 = 0U;
    uint8_t reg29 = 0U;
    int ret01;
    int ret0d;
    int ret28;
    int ret29;
    int log_len;

    if(HwAccess.em7028.read_reg == NULL) {
        return;
    }

    ret01 = HwAccess.em7028.read_reg(0x01U, &reg01);
    ret0d = HwAccess.em7028.read_reg(0x0DU, &reg0d);
    ret28 = HwAccess.em7028.read_reg(0x28U, &reg28);
    ret29 = HwAccess.em7028.read_reg(0x29U, &reg29);

    log_len = snprintf(log_buf,
                       sizeof(log_buf),
                       "REG 01=%d:0x%02X 0D=%d:0x%02X 28=%d:0x%02X 29=%d:0x%02X\r\n",
                       ret01,
                       reg01,
                       ret0d,
                       reg0d,
                       ret28,
                       reg28,
                       ret29,
                       reg29);
    if(log_len > 0) {
        HeartRate_Task_Log(log_buf);
    }
}

/**
 * @brief EM7028 原始 PPG 调试任务。
 *
 * 任务启动后直接开启 EM7028 连续采样，每 100ms 读取一次 16 位原始 ADC/PPG 数据，
 * 并通过项目现有蓝牙串口发送调试文本。
 */
void HeartRate_Task(void *argument)
{
    char log_buf[48];
    uint16_t raw_value;
    uint32_t tick_ms;
    int log_len;
    int ret;

    (void)argument;

    osDelay(HEART_RATE_TASK_STARTUP_DELAY_MS);

    if(HwAccess.bluetooth.enable != NULL) {
        HwAccess.bluetooth.enable();
    }
    osDelay(HEART_RATE_TASK_STARTUP_DELAY_MS);

    HeartRate_Task_Log("HeartRateTask start\r\n");

    if(HwAccess.em7028.init != NULL) {
        ret = HwAccess.em7028.init();
        if((HwAccess.em7028.get_pid != NULL) &&
           (HwAccess.em7028.get_probe_status != NULL)) {
            log_len = snprintf(log_buf,
                               sizeof(log_buf),
                               "EM7028 init=%d probe=%d pid=0x%02X\r\n",
                               ret,
                               HwAccess.em7028.get_probe_status(),
                               HwAccess.em7028.get_pid());
        } else {
            log_len = snprintf(log_buf, sizeof(log_buf), "EM7028 init=%d\r\n", ret);
        }
        if(log_len > 0) {
            HeartRate_Task_Log(log_buf);
        }
    }

    if(HwAccess.em7028.start != NULL) {
        ret = HwAccess.em7028.start();
        if((HwAccess.em7028.get_pid != NULL) &&
           (HwAccess.em7028.get_probe_status != NULL)) {
            log_len = snprintf(log_buf,
                               sizeof(log_buf),
                               "EM7028 start=%d probe=%d pid=0x%02X\r\n",
                               ret,
                               HwAccess.em7028.get_probe_status(),
                               HwAccess.em7028.get_pid());
        } else {
            log_len = snprintf(log_buf, sizeof(log_buf), "EM7028 start=%d\r\n", ret);
        }
        if(log_len > 0) {
            HeartRate_Task_Log(log_buf);
        }
    }

    HeartRate_Task_DumpRegisters();
    HeartRate_Task_Log("PPG raw stream begin\r\n");

    for(;;) {
        if((HwAccess.em7028.update_cache != NULL) &&
           (HwAccess.em7028.get_raw != NULL)) {
            ret = HwAccess.em7028.update_cache();
            if(ret == 0) {
                raw_value = HwAccess.em7028.get_raw();
                tick_ms = osKernelGetTickCount();
                log_len = snprintf(log_buf,
                                   sizeof(log_buf),
                                   "[%lu] EM7028_RAW=%u\r\n",
                                   (unsigned long)tick_ms,
                                   raw_value);
            } else {
                log_len = snprintf(log_buf, sizeof(log_buf), "EM7028 read=%d\r\n", ret);
            }

            if(log_len > 0) {
                HeartRate_Task_Log(log_buf);
            }
        }

        osDelay(HEART_RATE_TASK_SAMPLE_PERIOD_MS);
    }
}
