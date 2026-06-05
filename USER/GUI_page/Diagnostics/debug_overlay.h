#ifndef DEBUG_OVERLAY_H
#define DEBUG_OVERLAY_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化全局调试浮层。
 *
 * 该浮层挂在 LVGL top layer 上，不属于任何单个页面，因此页面切换时会持续显示。
 * 只能在 GUI/LVGL 任务上下文中调用。
 */
void DebugOverlay_Init(void);

#ifdef __cplusplus
}
#endif

#endif /* DEBUG_OVERLAY_H */
