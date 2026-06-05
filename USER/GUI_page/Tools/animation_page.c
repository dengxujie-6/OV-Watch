/**
 * @file animation_page.c
 * @brief FPS 动画测试页面实现。
 */

#include "animation_page.h"

#include <string.h>

#define ANIMATION_PAGE_DOT_COUNT       8U
#define ANIMATION_PAGE_TIMER_MS        33U
#define ANIMATION_PAGE_TRACK_WIDTH     176
#define ANIMATION_PAGE_TRACK_LEFT      26
#define ANIMATION_PAGE_FIRST_TRACK_Y   68
#define ANIMATION_PAGE_TRACK_GAP       18

struct animation_page {
    lv_obj_t * root;                         /**< 页面根 screen，归本页面对象所有。 */
    lv_obj_t * title_label;                  /**< 页面标题，root 删除时自动删除。 */
    lv_obj_t * dots[ANIMATION_PAGE_DOT_COUNT]; /**< 运动圆点数组，root 删除时自动删除。 */
    lv_timer_t * refresh_timer;              /**< 动画刷新定时器，由本页面删除。 */
    uint16_t phase;                          /**< 动画相位，定时器中递增。 */
};

static animation_page_t * s_animation_page;

static void animation_page_timer_cb(lv_timer_t * timer);
static int16_t animation_page_triangle(uint16_t phase, uint16_t period, int16_t amplitude);
static void animation_page_update(animation_page_t * page);

/**
 * @brief 计算三角波位移。
 *
 * @param phase 当前相位。
 * @param period 周期，必须大于 0。
 * @param amplitude 输出峰值。
 * @return 范围为 0~amplitude 的位移。
 */
static int16_t animation_page_triangle(uint16_t phase, uint16_t period, int16_t amplitude)
{
    uint16_t pos;
    uint32_t value;

    if(period == 0U) {
        return 0;
    }

    pos = (uint16_t)(phase % period);
    if(pos > (period / 2U)) {
        pos = (uint16_t)(period - pos);
    }

    value = ((uint32_t)pos * 2UL * (uint32_t)amplitude) / (uint32_t)period;
    return (int16_t)value;
}

/**
 * @brief 定时推进动画对象位置和颜色。
 */
static void animation_page_timer_cb(lv_timer_t * timer)
{
    animation_page_t * page = (animation_page_t *)lv_timer_get_user_data(timer);

    if(page == NULL) {
        return;
    }

    page->phase = (uint16_t)(page->phase + 9U);
    animation_page_update(page);
}

/**
 * @brief 刷新动画对象。
 */
static void animation_page_update(animation_page_t * page)
{
    if((page == NULL) || (page->root == NULL)) {
        return;
    }

    for(uint8_t i = 0U; i < ANIMATION_PAGE_DOT_COUNT; i++) {
        uint16_t dot_phase = (uint16_t)(page->phase + (uint16_t)(i * 28U));
        int16_t x = animation_page_triangle(dot_phase, 360U, ANIMATION_PAGE_TRACK_WIDTH);
        int16_t y = (int16_t)(ANIMATION_PAGE_FIRST_TRACK_Y + ((int16_t)i * ANIMATION_PAGE_TRACK_GAP));

        if(page->dots[i] == NULL) {
            continue;
        }

        // 只移动小对象，避免半透明大面积对象把局部刷新合并成接近整屏刷新。
        lv_obj_set_pos(page->dots[i], ANIMATION_PAGE_TRACK_LEFT + x, y);
    }
}

animation_page_t * animation_page_create(void)
{
    static animation_page_t page_storage;
    animation_page_t * page = &page_storage;

    memset(page, 0, sizeof(*page));

    page->root = lv_obj_create(NULL);
    if(page->root == NULL) {
        return NULL;
    }

    lv_obj_remove_style_all(page->root);
    lv_obj_set_size(page->root, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(page->root, lv_color_hex(0x070a0f), 0);
    lv_obj_set_style_bg_opa(page->root, LV_OPA_COVER, 0);
    lv_obj_clear_flag(page->root, LV_OBJ_FLAG_SCROLLABLE);

    page->title_label = lv_label_create(page->root);
    lv_label_set_text(page->title_label, "FPS Animation");
    lv_obj_set_style_text_color(page->title_label, lv_color_hex(0xdde6f3), 0);
    lv_obj_set_style_text_font(page->title_label, LV_FONT_DEFAULT, 0);
    lv_obj_align(page->title_label, LV_ALIGN_TOP_LEFT, 12, 12);

    for(uint8_t i = 0U; i < ANIMATION_PAGE_DOT_COUNT; i++) {
        lv_obj_t * dot = lv_obj_create(page->root);
        page->dots[i] = dot;
        if(dot == NULL) {
            continue;
        }

        lv_obj_remove_style_all(dot);
        lv_obj_set_size(dot, 14, 14);
        lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(dot,
                                  lv_color_hsv_to_rgb((uint16_t)(i * 360U / ANIMATION_PAGE_DOT_COUNT),
                                                      80,
                                                      92),
                                  0);
    }

    animation_page_update(page);

    page->refresh_timer = lv_timer_create(animation_page_timer_cb,
                                          ANIMATION_PAGE_TIMER_MS,
                                          page);
    return page;
}

void animation_page_destroy(animation_page_t * page)
{
    if(page == NULL) {
        return;
    }

    if(page->refresh_timer != NULL) {
        lv_timer_del(page->refresh_timer);
        page->refresh_timer = NULL;
    }

    if(page->root != NULL) {
        lv_obj_del(page->root);
    }

    memset(page, 0, sizeof(*page));
}

lv_obj_t * animation_page_root(animation_page_t * page)
{
    return (page != NULL) ? page->root : NULL;
}

/**
 * @brief 创建并加载 FPS 动画测试页面。
 */
static void AnimationPage_Create(void)
{
    lv_obj_t * root;

    if(s_animation_page != NULL) {
        animation_page_destroy(s_animation_page);
        s_animation_page = NULL;
    }

    s_animation_page = animation_page_create();
    if(s_animation_page == NULL) {
        return;
    }

    root = animation_page_root(s_animation_page);
    if(root == NULL) {
        animation_page_destroy(s_animation_page);
        s_animation_page = NULL;
        return;
    }

    lv_screen_load(root);
}

/**
 * @brief 释放 FPS 动画测试页面。
 */
static void AnimationPage_Destroy(void)
{
    animation_page_destroy(s_animation_page);
    s_animation_page = NULL;
}

const GUI_Page_t AnimationPage = {
    .create = AnimationPage_Create,
    .destroy = AnimationPage_Destroy,
};
