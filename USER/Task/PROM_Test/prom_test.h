#ifndef PROM_TEST_H
#define PROM_TEST_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef PROM_TEST_ENABLE
#define PROM_TEST_ENABLE       0
#endif

#define PROM_TEST_START_ADDR   0xE0U
#define PROM_TEST_LENGTH       32U

#define PROM_TEST_OK           0
#define PROM_TEST_ERR_PROBE   (-1)
#define PROM_TEST_ERR_BACKUP  (-2)
#define PROM_TEST_ERR_WRITE   (-3)
#define PROM_TEST_ERR_READ    (-4)
#define PROM_TEST_ERR_VERIFY  (-5)
#define PROM_TEST_ERR_RESTORE (-6)
#define PROM_TEST_RUNNING      126
#define PROM_TEST_DISABLED     127

typedef struct
{
    int status;
    int restore_status;
    uint8_t start_addr;
    uint8_t length;
    uint8_t fail_index;
    uint8_t expected;
    uint8_t actual;
} PROM_Test_Result_t;

extern volatile PROM_Test_Result_t g_prom_test_result;
extern uint8_t g_prom_test_write_data[PROM_TEST_LENGTH];
extern uint8_t g_prom_test_read_data[PROM_TEST_LENGTH];

/**
 * @brief 执行一次 BL24C02F PROM 写入和读回测试。
 *
 * 测试会把 g_prom_test_write_data 写入 PROM，再读回到 g_prom_test_read_data。
 * 该函数只能在任务上下文调用，不能在中断里调用。
 *
 * @return PROM_TEST_OK 表示通过，负数表示失败阶段。
 */
int PROM_Test_Run(void);

/**
 * @brief 获取最近一次 PROM 测试结果。
 *
 * @return 指向全局结果结构体的指针，调用者只读，不拥有该对象。
 */
const volatile PROM_Test_Result_t * PROM_Test_GetResult(void);

#ifdef __cplusplus
}
#endif

#endif /* PROM_TEST_H */
