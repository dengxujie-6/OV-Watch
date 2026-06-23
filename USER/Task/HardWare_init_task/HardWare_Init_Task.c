#include "HardWare_Init_Task.h"

#include "cmsis_os2.h"
#include "hwaccess.h"
#include "prom_test.h"

volatile uint32_t HardwareInit_DebugStage;
volatile int HardwareInit_PromTestStatus = PROM_TEST_DISABLED;
volatile PROM_Test_Result_t HardwareInit_PromTestResult;
volatile uint8_t HardwareInit_PromWriteData[PROM_TEST_LENGTH];
volatile uint8_t HardwareInit_PromReadData[PROM_TEST_LENGTH];

#if PROM_TEST_ENABLE
static void HardwareInit_CopyPromTestResult(const volatile PROM_Test_Result_t * result);
static void HardwareInit_CopyPromTestData(void);

/**
 * @brief 把 PROM 测试结果复制到硬件初始化任务自己的全局变量里。
 *
 * Keil Watch 有时不容易直接找到未在当前模块引用的全局符号，所以这里把结果镜像到
 * HardWare_Init_Task.c，和 HardwareInit_DebugStage 放在同一个调试上下文里。
 */
static void HardwareInit_CopyPromTestResult(const volatile PROM_Test_Result_t * result)
{
    if(result == 0) {
        return;
    }

    HardwareInit_PromTestResult.status = result->status;
    HardwareInit_PromTestResult.restore_status = result->restore_status;
    HardwareInit_PromTestResult.start_addr = result->start_addr;
    HardwareInit_PromTestResult.length = result->length;
    HardwareInit_PromTestResult.fail_index = result->fail_index;
    HardwareInit_PromTestResult.expected = result->expected;
    HardwareInit_PromTestResult.actual = result->actual;
}

/**
 * @brief 把 PROM 写入数组和读回数组复制到硬件初始化任务全局变量里。
 */
static void HardwareInit_CopyPromTestData(void)
{
    uint8_t i;

    for(i = 0U; i < PROM_TEST_LENGTH; i++) {
        HardwareInit_PromWriteData[i] = g_prom_test_write_data[i];
        HardwareInit_PromReadData[i] = g_prom_test_read_data[i];
    }
}
#endif

/**
 * @brief Initialize board hardware.
 *
 * 该任务只负责硬件启动顺序，任务创建由 freertos.c 统一完成。
 */
void HardWare_Init_Task(void *argument)
{
    (void)argument;

    HwAccess.watchdog.init();

    HardwareInit_DebugStage = 1U;
    HwAccess.power.open();
    HardwareInit_DebugStage = 2U;
    HwAccess.lcd.init();
    HardwareInit_DebugStage = 3U;
    HwAccess.bluetooth.init();
    HardwareInit_DebugStage = 4U;
    if(HwAccess.em7028.init != 0) {
        (void)HwAccess.em7028.init();
    }
    HardwareInit_DebugStage = 5U;
#if PROM_TEST_ENABLE
    HardwareInit_PromTestStatus = PROM_TEST_RUNNING;
    // PROM 自检会写 EEPROM，默认关闭；需要实机验证时在 prom_test.h 中打开宏。
    HardwareInit_PromTestStatus = PROM_Test_Run();
    HardwareInit_CopyPromTestResult(PROM_Test_GetResult());
    HardwareInit_CopyPromTestData();
#else
    HardwareInit_PromTestStatus = PROM_TEST_DISABLED;
#endif
    HardwareInit_DebugStage = 6U;

    osThreadExit();
}
