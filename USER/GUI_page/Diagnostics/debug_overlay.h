#ifndef DEBUG_OVERLAY_H
#define DEBUG_OVERLAY_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化 GUI 调试统计。
 *
 * 该模块只统计 FPS，不创建跨页面显示对象；测试页面负责读取并显示统计值。
 * 只能在 GUI/LVGL 任务上下文中调用。
 */
void DebugOverlay_Init(void);

/**
 * @brief 读取最近一次计算得到的 FPS。
 *
 * @return 最近一个统计周期的刷新帧率。
 */
uint32_t DebugOverlay_GetFps(void);

/**
 * @brief 读取 LVGL 内存池总量和已用量。
 *
 * @param total_bytes 输出总字节数，允许为 NULL。
 * @param used_bytes 输出已用字节数，允许为 NULL。
 */
void DebugOverlay_GetLvMem(uint32_t * total_bytes, uint32_t * used_bytes);

#ifdef __cplusplus
}
#endif

#endif /* DEBUG_OVERLAY_H */
