#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <stddef.h>
#include <stdint.h>

extern uint8_t _end;
extern uint8_t _estack;

static int g_newlib_errno;
static uint8_t *g_heap_end;

/**
 * @brief 为 newlib/newlib-nano 提供 errno 存储入口。
 *
 * GCC + libm 的部分实现会直接引用 `__errno()`。
 * 当前 GCC/CMake 固件不启用 newlib reent，也不走文件系统，
 * 因此这里提供一个最小的全局 errno 存储即可满足链接与运行需要。
 *
 * @return int* 指向当前全局 errno 的指针。
 */
int *__errno(void)
{
    return &g_newlib_errno;
}

/**
 * @brief 为少量 libc 分配场景提供最小 sbrk 实现。
 *
 * 该工程主要使用 FreeRTOS heap_4 与 LVGL 内存池，不依赖 newlib 堆。
 * 这里仅提供一个保守的线性堆指针，避免 GCC 裸机链接时出现 `_sbrk`
 * 缺失；若越过栈底，则返回失败并设置 errno。
 *
 * @param incr 请求扩展的字节数。
 * @return void* 成功时返回旧堆顶，失败时返回 `(void *)-1`。
 */
void *_sbrk(ptrdiff_t incr)
{
    uint8_t *prev_heap_end;
    uint8_t *next_heap_end;

    if (g_heap_end == NULL) {
        g_heap_end = &_end;
    }

    prev_heap_end = g_heap_end;
    next_heap_end = g_heap_end + incr;

    if (next_heap_end >= (&_estack - 0x400U)) {
        g_newlib_errno = ENOMEM;
        return (void *)-1;
    }

    g_heap_end = next_heap_end;
    return prev_heap_end;
}

/**
 * @brief 裸机固件不支持文件关闭，统一返回失败。
 */
int _close(int file)
{
    (void)file;
    g_newlib_errno = ENOSYS;
    return -1;
}

/**
 * @brief 裸机固件不支持文件读，统一返回失败。
 */
int _read(int file, char *ptr, int len)
{
    (void)file;
    (void)ptr;
    (void)len;
    g_newlib_errno = ENOSYS;
    return -1;
}

/**
 * @brief 裸机固件不支持文件定位，统一返回失败。
 */
int _lseek(int file, int ptr, int dir)
{
    (void)file;
    (void)ptr;
    (void)dir;
    g_newlib_errno = ENOSYS;
    return -1;
}

/**
 * @brief 裸机固件默认不接标准输出，直接丢弃写入数据。
 *
 * 这样可以让 `printf` 等接口在 GCC 调试构建下完成链接，
 * 同时避免强行绑定某个串口，保持与现有板级日志路径解耦。
 */
int _write(int file, const char *ptr, int len)
{
    (void)file;
    (void)ptr;
    return len;
}

/**
 * @brief 告知 newlib 当前文件描述符按字符设备处理。
 */
int _isatty(int file)
{
    (void)file;
    return 1;
}

/**
 * @brief 提供最小 stat 结果，满足 libc 查询。
 */
int _fstat(int file, struct stat *st)
{
    (void)file;

    if (st == NULL) {
        g_newlib_errno = EINVAL;
        return -1;
    }

    st->st_mode = S_IFCHR;
    return 0;
}

/**
 * @brief 裸机固件退出时停在死循环，等待调试器介入。
 */
void _exit(int status)
{
    (void)status;

    while (1) {
    }
}

