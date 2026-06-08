#include "prom_test.h"

#include "hwaccess.h"

uint8_t g_prom_test_write_data[PROM_TEST_LENGTH] = {
    0x24U, 0xC0U, 0x02U, 0x5AU, 0xA5U, 0x11U, 0x22U, 0x33U,
    0x44U, 0x55U, 0x66U, 0x77U, 0x88U, 0x99U, 0xAAU, 0xBBU,
    0xCCU, 0xDDU, 0xEEU, 0xFFU, 0x00U, 0x10U, 0x20U, 0x30U,
    0x40U, 0x50U, 0x60U, 0x70U, 0x80U, 0x90U, 0xB1U, 0x4EU,
};

uint8_t g_prom_test_read_data[PROM_TEST_LENGTH];

volatile PROM_Test_Result_t g_prom_test_result;

static void PROM_Test_ResetResult(void);
static void PROM_Test_ClearReadData(void);
static int PROM_Test_Verify(void);

/**
 * @brief 清空 PROM 测试结果，方便调试器观察本次运行状态。
 */
static void PROM_Test_ResetResult(void)
{
    g_prom_test_result.status = PROM_TEST_OK;
    g_prom_test_result.restore_status = PROM_TEST_OK;
    g_prom_test_result.start_addr = PROM_TEST_START_ADDR;
    g_prom_test_result.length = PROM_TEST_LENGTH;
    g_prom_test_result.fail_index = 0U;
    g_prom_test_result.expected = 0U;
    g_prom_test_result.actual = 0U;
}

/**
 * @brief 清空读回数组，避免调试时误看上一次残留值。
 */
static void PROM_Test_ClearReadData(void)
{
    uint8_t i;

    for(i = 0U; i < PROM_TEST_LENGTH; i++) {
        g_prom_test_read_data[i] = 0U;
    }
}

/**
 * @brief 对比写入数组和读回数组，并记录第一个不一致的字节。
 */
static int PROM_Test_Verify(void)
{
    uint8_t i;

    for(i = 0U; i < PROM_TEST_LENGTH; i++) {
        if(g_prom_test_write_data[i] != g_prom_test_read_data[i]) {
            g_prom_test_result.fail_index = i;
            g_prom_test_result.expected = g_prom_test_write_data[i];
            g_prom_test_result.actual = g_prom_test_read_data[i];
            return PROM_TEST_ERR_VERIFY;
        }
    }

    return PROM_TEST_OK;
}

/**
 * @brief 执行一次 BL24C02F PROM 写入和读回测试。
 */
int PROM_Test_Run(void)
{
    int ret;

    PROM_Test_ResetResult();
    PROM_Test_ClearReadData();

    if((HwAccess.prom.init == 0) ||
       (HwAccess.prom.probe == 0) ||
       (HwAccess.prom.read == 0) ||
       (HwAccess.prom.write == 0)) {
        g_prom_test_result.status = PROM_TEST_ERR_PROBE;
        return g_prom_test_result.status;
    }

    HwAccess.prom.init();

    ret = HwAccess.prom.probe();
    if(ret != 0) {
        g_prom_test_result.status = PROM_TEST_ERR_PROBE;
        return g_prom_test_result.status;
    }

    // 0xE0~0xFF 共 32 字节，跨两个 16 字节页，用来同时验证分页写逻辑。
    ret = HwAccess.prom.write(PROM_TEST_START_ADDR, g_prom_test_write_data, PROM_TEST_LENGTH);
    if(ret != 0) {
        g_prom_test_result.status = PROM_TEST_ERR_WRITE;
        return g_prom_test_result.status;
    }

    ret = HwAccess.prom.read(PROM_TEST_START_ADDR, g_prom_test_read_data, PROM_TEST_LENGTH);
    if(ret != 0) {
        g_prom_test_result.status = PROM_TEST_ERR_READ;
        return g_prom_test_result.status;
    }

    ret = PROM_Test_Verify();
    if(ret != PROM_TEST_OK) {
        g_prom_test_result.status = ret;
        return g_prom_test_result.status;
    }

    g_prom_test_result.status = PROM_TEST_OK;
    return PROM_TEST_OK;
}

/**
 * @brief 获取最近一次 PROM 测试结果。
 */
const volatile PROM_Test_Result_t * PROM_Test_GetResult(void)
{
    return &g_prom_test_result;
}
