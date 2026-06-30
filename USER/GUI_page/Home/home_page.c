/**
 * @file home_page.c
 * @brief 默认主页页面实现。
 *
 * 页面仅使用 LVGL 对象绘制首页，不直接访问 HAL/BSP。步数、温湿度和电量
 * 都通过 HwAccess 缓存读取，由后台任务负责真实采样和计步。
 * 温湿度和电量通过 HwAccess 缓存读取，由后台任务负责真实采样。
 */

#include "home_page.h"

#include <string.h>

#include "dropdown_menu_page.h"
#include "hwaccess.h"
#include "menu_page.h"
#include "my_icon_fonts.h"

extern const lv_font_t my_font_source_han_10;
extern const lv_font_t my_font_source_han_13;
extern const lv_font_t my_font_source_han_18;
extern const lv_font_t my_font_source_han_19;
extern const lv_font_t my_font_source_han_24;
extern const lv_font_t my_font_source_han_38;

// 屏幕逻辑尺寸，主页所有坐标都按 240 x 280 绝对像素布局。
#define HOME_SCREEN_W 240
#define HOME_SCREEN_H 280
// 主页整体视觉内容下移 4px，卡片内部子控件保持相对布局不变。
#define HOME_UI_Y_OFFSET 4
// 左划进入菜单的跟手距离阈值，超过后松手进入菜单页。
#define HOME_MENU_DRAG_COMMIT_X 80
// 手指轻微抖动不立即显示菜单预览层，避免误触。
#define HOME_MENU_DRAG_START_X 8
// CST816T 滑动坐标变化偏小，菜单层用 1.5 倍位移追上手指。
#define HOME_MENU_DRAG_GAIN_NUM 3
#define HOME_MENU_DRAG_GAIN_DEN 2
// 主页激活时轮询 pointer 输入，保证菜单跟手不依赖事件命中到根对象。
#define HOME_MENU_DRAG_TIMER_MS 4U
// 慢速滑动时触摸芯片可能短暂报松手，连续确认后才触发回弹/进入。
#define HOME_MENU_RELEASE_CONFIRM_COUNT 3U
#define HOME_DROPDOWN_DRAG_COMMIT_Y 72
#define HOME_DROPDOWN_DRAG_START_Y 10
#define HOME_DROPDOWN_EDGE_LIMIT_Y 36
#define HOME_BATTERY_REFRESH_MS 1000U
#define HOME_STEP_TARGET        15000U

// 主页配色集中放在这里，后续微调视觉风格时优先改这些颜色。
#define HOME_COLOR_BG        0x02070c
#define HOME_COLOR_PANEL     0x07131e
#define HOME_COLOR_WHITE     0xffffff
#define HOME_COLOR_TEXT_GRAY 0x8b96aa
#define HOME_COLOR_GREEN     0x49f59b
#define HOME_COLOR_ARC_BG    0x1b252d
#define HOME_COLOR_ORANGE    0xffbd3c
#define HOME_COLOR_BLUE      0x4eb6ff

/**
 * @brief 默认主页页面对象。
 *
 * root 是常驻页面根 screen。PageManager 栈底规则会保留该页面对象，进入菜单或其它页面时
 * 仅切换 screen，不删除 root；系统退出或异常清理时仍可调用 destroy 完整释放。
 */
struct home_page {
    lv_obj_t * root;        /**< 页面根 screen，归本页面对象所有。 */
    lv_obj_t * step_arc;    /**< 步数圆环，root 删除时自动删除。 */
    lv_obj_t * step_label;  /**< 步数字符，root 删除时自动删除。 */
    lv_obj_t * battery_level; /**< 顶部电池电量条，root 删除时自动删除。 */
    lv_obj_t * battery_label; /**< 顶部电池百分比文字，root 删除时自动删除。 */
    lv_obj_t * temperature_label; /**< 温度卡片数值，root 删除时自动删除。 */
    lv_obj_t * humidity_label; /**< 湿度卡片数值，root 删除时自动删除。 */
    lv_obj_t * menu_preview; /**< 左划跟手阶段的临时菜单预览层，root 删除时自动删除。 */
    lv_obj_t * dropdown_preview; /**< 下滑阶段的临时快捷菜单预览层，root 删除时自动删除。 */
    lv_timer_t * drag_timer; /**< 主页激活时的触摸轮询定时器，由本页面删除。 */
    lv_timer_t * battery_timer; /**< 低频刷新主页缓存数据显示的 LVGL 定时器。 */
    lv_point_t press_point; /**< 本次触摸按下坐标，用于计算横向拖动距离。 */
    int32_t drag_x;         /**< 当前左划距离，单位为屏幕像素。 */
    int32_t drag_y;         /**< 当前下滑距离，单位为屏幕像素。 */
    uint8_t release_count;  /**< 连续检测到松手的次数，用于过滤触摸采样抖动。 */
    bool touch_pressed;     /**< 上一次轮询时 pointer 是否处于按下状态。 */
    bool drag_active;       /**< 已进入左划跟手状态。 */
    bool dropdown_drag_active; /**< 已进入下滑跟手状态。 */
    bool transition_active; /**< 松手后的收尾动画正在运行。 */
};

static home_page_t * s_home_page;
static const char * s_home_menu_preview_titles[] = {
    "日历", "计算器", "秒表", "动画", "卡包", "运动", "心率",
    "血氧", "环境", "游戏", "设置", "关于",
};
static const char * s_home_menu_preview_icons[] = {
    LV_SYMBOL_LIST, LV_SYMBOL_SETTINGS, LV_SYMBOL_REFRESH, LV_SYMBOL_PLAY,
    LV_SYMBOL_DIRECTORY, LV_SYMBOL_UP, LV_SYMBOL_TINT, LV_SYMBOL_AUDIO,
    LV_SYMBOL_HOME, LV_SYMBOL_PLAY, LV_SYMBOL_SETTINGS,
    LV_SYMBOL_COPY,
};
static const uint32_t s_home_menu_preview_icon_colors[] = {
    0xff6b6b, 0xffa94d, 0xb197fc, 0x4dabf7, 0xffd43b, 0x51cf66, 0xff8787,
    0x74c0fc, 0x63e6be, 0xffc078, 0xadb5bd, 0xdee2e6,
};

static lv_indev_t * home_pointer_indev_get(void);
static void home_menu_drag_poll(home_page_t * page, lv_indev_t * indev);
static void home_menu_drag_timer_cb(lv_timer_t * timer);
static void home_dropdown_preview_create(home_page_t * page);
static void home_dropdown_preview_set_drag(home_page_t * page, int32_t drag_y);
static void home_dropdown_preview_hide(home_page_t * page);
static void home_dropdown_drag_finish(home_page_t * page, bool commit);
static void home_battery_timer_cb(lv_timer_t * timer);
static void home_battery_update(home_page_t * page);
static void home_steps_update(home_page_t * page);
static void home_sensor_update(home_page_t * page);

static void home_page_key_cb(lv_event_t * e)
{
    if(lv_event_get_code(e) != LV_EVENT_KEY) return;
    if(lv_event_get_key(e) == LV_KEY_ENTER) {
        (void)PageManager_Push(&MenuPage);
    }
}

static lv_indev_t * home_pointer_indev_get(void)
{
    lv_indev_t * indev = NULL;

    while((indev = lv_indev_get_next(indev)) != NULL) {
        if(lv_indev_get_type(indev) == LV_INDEV_TYPE_POINTER) {
            return indev;
        }
    }

    return NULL;
}

static void home_menu_preview_delete(home_page_t * page)
{
    if((page == NULL) || (page->menu_preview == NULL)) return;

    lv_obj_del(page->menu_preview);
    page->menu_preview = NULL;
}

static void home_dropdown_preview_delete(home_page_t * page)
{
    if((page == NULL) || (page->dropdown_preview == NULL)) return;

    lv_obj_del(page->dropdown_preview);
    page->dropdown_preview = NULL;
}

static void home_page_reset_root_pos(home_page_t * page)
{
    if((page == NULL) || (page->root == NULL)) return;

    // 禁用根对象滚动后仍主动复位，避免取消左划时残留输入系统造成的位置偏移。
    lv_obj_set_pos(page->root, 0, 0);
    lv_obj_scroll_to(page->root, 0, 0, LV_ANIM_OFF);
}

static void home_dropdown_anim_ready_cb(lv_anim_t * a)
{
    home_page_t * page = (home_page_t *)lv_anim_get_user_data(a);

    if(page == NULL) return;

    home_page_reset_root_pos(page);
    home_dropdown_preview_hide(page);
    page->dropdown_drag_active = false;
    page->transition_active = false;
    page->touch_pressed = false;
    page->release_count = 0U;
    page->drag_y = 0;
    (void)PageManager_Push(&DropdownMenuPage);
}

static void home_dropdown_cancel_anim_ready_cb(lv_anim_t * a)
{
    home_page_t * page = (home_page_t *)lv_anim_get_user_data(a);

    if(page == NULL) return;

    home_page_reset_root_pos(page);
    home_dropdown_preview_hide(page);
    page->dropdown_drag_active = false;
    page->transition_active = false;
    page->touch_pressed = false;
    page->release_count = 0U;
    page->drag_y = 0;
}

static void home_dropdown_preview_anim_start(home_page_t * page,
                                             int32_t target_y,
                                             lv_anim_completed_cb_t completed_cb)
{
    lv_anim_t anim;

    if((page == NULL) || (page->dropdown_preview == NULL)) return;

    page->transition_active = true;
    lv_anim_init(&anim);
    lv_anim_set_var(&anim, page->dropdown_preview);
    lv_anim_set_values(&anim, lv_obj_get_y(page->dropdown_preview), target_y);
    lv_anim_set_duration(&anim, 120);
    lv_anim_set_exec_cb(&anim, (lv_anim_exec_xcb_t)lv_obj_set_y);
    lv_anim_set_user_data(&anim, page);
    lv_anim_set_completed_cb(&anim, completed_cb);
    lv_anim_start(&anim);
}

static lv_obj_t * home_menu_preview_item_create(lv_obj_t * parent, size_t index)
{
    lv_obj_t * btn = lv_obj_create(parent);
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_width(btn, lv_pct(100));
    lv_obj_set_height(btn, 70);
    lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(btn, 0, 0);
    lv_obj_set_style_radius(btn, 0, 0);
    lv_obj_set_style_pad_hor(btn, 18, 0);
    lv_obj_set_style_pad_ver(btn, 10, 0);

    lv_obj_t * icon_bg = lv_obj_create(btn);
    lv_obj_clear_flag(icon_bg, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(icon_bg, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(icon_bg, 46, 46);
    lv_obj_set_style_radius(icon_bg, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(icon_bg, lv_color_hex(s_home_menu_preview_icon_colors[index]), 0);
    lv_obj_set_style_bg_opa(icon_bg, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(icon_bg, 0, 0);
    lv_obj_set_style_pad_all(icon_bg, 0, 0);
    lv_obj_align(icon_bg, LV_ALIGN_LEFT_MID, 0, 0);

    lv_obj_t * icon = lv_label_create(icon_bg);
    lv_label_set_text(icon, s_home_menu_preview_icons[index]);
    lv_obj_set_style_text_color(icon, lv_color_white(), 0);
    lv_obj_center(icon);

    lv_obj_t * title = lv_label_create(btn);
    lv_label_set_text(title, s_home_menu_preview_titles[index]);
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_set_style_text_font(title, &my_font_source_han_24, 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 62, 0);

    return btn;
}

static void home_menu_preview_create(home_page_t * page)
{
    if((page == NULL) || (page->root == NULL) || (page->menu_preview != NULL)) return;

    page->menu_preview = lv_obj_create(page->root);
    lv_obj_remove_style_all(page->menu_preview);
    lv_obj_set_size(page->menu_preview, HOME_SCREEN_W, HOME_SCREEN_H);
    lv_obj_set_pos(page->menu_preview, HOME_SCREEN_W, 0);
    lv_obj_set_style_bg_color(page->menu_preview, lv_color_hex(0x0b0f12), 0);
    lv_obj_set_style_bg_opa(page->menu_preview, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(page->menu_preview, 0, 0);
    lv_obj_set_style_pad_all(page->menu_preview, 8, 0);
    lv_obj_clear_flag(page->menu_preview, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(page->menu_preview, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(page->menu_preview, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t * list = lv_obj_create(page->menu_preview);
    lv_obj_remove_style_all(list);
    lv_obj_set_size(list, HOME_SCREEN_W - 16, HOME_SCREEN_H - 16);
    lv_obj_set_style_bg_opa(list, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(list, 0, 0);
    lv_obj_set_style_pad_all(list, 0, 0);
    lv_obj_set_style_pad_row(list, 8, 0);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(list, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(list, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_center(list);

    for(size_t i = 0; i < (sizeof(s_home_menu_preview_titles) / sizeof(s_home_menu_preview_titles[0])); i++) {
        (void)home_menu_preview_item_create(list, i);
    }
}

static void home_dropdown_preview_create(home_page_t * page)
{
    if((page == NULL) || (page->root == NULL) || (page->dropdown_preview != NULL)) return;

    page->dropdown_preview = lv_obj_create(page->root);
    lv_obj_remove_style_all(page->dropdown_preview);
    lv_obj_set_size(page->dropdown_preview, HOME_SCREEN_W, 132);
    lv_obj_set_pos(page->dropdown_preview, 0, -132);
    lv_obj_set_style_bg_color(page->dropdown_preview, lv_color_hex(0x101820), 0);
    lv_obj_set_style_bg_opa(page->dropdown_preview, LV_OPA_90, 0);
    lv_obj_set_style_border_width(page->dropdown_preview, 0, 0);
    lv_obj_set_style_radius(page->dropdown_preview, 0, 0);
    lv_obj_set_style_pad_all(page->dropdown_preview, 16, 0);
    lv_obj_clear_flag(page->dropdown_preview, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(page->dropdown_preview, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(page->dropdown_preview, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t * title = lv_label_create(page->dropdown_preview);
    lv_label_set_text(title, "快捷菜单");
    lv_obj_set_style_text_font(title, &my_font_source_han_24, 0);
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_t * panel = lv_obj_create(page->dropdown_preview);
    lv_obj_set_size(panel, HOME_SCREEN_W - 32, 64);
    lv_obj_align(panel, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(panel, lv_color_hex(0x17232d), 0);
    lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(panel, 0, 0);
    lv_obj_set_style_radius(panel, 18, 0);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t * icon = lv_label_create(panel);
    lv_label_set_text(icon, LV_SYMBOL_BLUETOOTH);
    lv_obj_set_style_text_font(icon, &my_font_source_han_24, 0);
    lv_obj_set_style_text_color(icon, lv_color_hex(0x4eb6ff), 0);
    lv_obj_align(icon, LV_ALIGN_LEFT_MID, 14, 0);

    lv_obj_t * label = lv_label_create(panel);
    lv_label_set_text(label, "蓝牙");
    lv_obj_set_style_text_font(label, &my_font_source_han_24, 0);
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    lv_obj_align(label, LV_ALIGN_LEFT_MID, 48, 0);
}

static void home_menu_preview_set_drag(home_page_t * page, int32_t drag_x)
{
    int32_t visual_drag_x;

    if((page == NULL) || (page->menu_preview == NULL)) return;

    if(drag_x < 0) drag_x = 0;
    visual_drag_x = (drag_x * HOME_MENU_DRAG_GAIN_NUM) / HOME_MENU_DRAG_GAIN_DEN;
    if(visual_drag_x > HOME_SCREEN_W) visual_drag_x = HOME_SCREEN_W;
    page->drag_x = visual_drag_x;

    // 菜单预览层按放大后的视觉位移进入，让页面边缘更贴近手指位置。
    lv_obj_clear_flag(page->menu_preview, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_x(page->menu_preview, HOME_SCREEN_W - visual_drag_x);
}

static void home_menu_preview_hide(home_page_t * page)
{
    if((page == NULL) || (page->menu_preview == NULL)) return;

    lv_obj_set_x(page->menu_preview, HOME_SCREEN_W);
    lv_obj_add_flag(page->menu_preview, LV_OBJ_FLAG_HIDDEN);
}

static void home_dropdown_preview_set_drag(home_page_t * page, int32_t drag_y)
{
    if((page == NULL) || (page->dropdown_preview == NULL)) return;

    if(drag_y < 0) drag_y = 0;
    if(drag_y > 132) drag_y = 132;
    page->drag_y = drag_y;

    lv_obj_clear_flag(page->dropdown_preview, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_y(page->dropdown_preview, drag_y - 132);
}

static void home_dropdown_preview_hide(home_page_t * page)
{
    if((page == NULL) || (page->dropdown_preview == NULL)) return;

    lv_obj_set_y(page->dropdown_preview, -132);
    lv_obj_add_flag(page->dropdown_preview, LV_OBJ_FLAG_HIDDEN);
}

static void home_menu_anim_ready_cb(lv_anim_t * a)
{
    home_page_t * page = (home_page_t *)lv_anim_get_user_data(a);

    if(page == NULL) return;

    home_page_reset_root_pos(page);
    home_menu_preview_hide(page);
    page->drag_active = false;
    page->transition_active = false;
    page->touch_pressed = false;
    page->release_count = 0U;
    page->drag_x = 0;
    (void)PageManager_Push(&MenuPage);
}

static void home_menu_cancel_anim_ready_cb(lv_anim_t * a)
{
    home_page_t * page = (home_page_t *)lv_anim_get_user_data(a);

    if(page == NULL) return;

    home_page_reset_root_pos(page);
    home_menu_preview_hide(page);
    page->drag_active = false;
    page->transition_active = false;
    page->touch_pressed = false;
    page->release_count = 0U;
    page->drag_x = 0;
}

static void home_menu_preview_anim_start(home_page_t * page, int32_t target_x, lv_anim_completed_cb_t completed_cb)
{
    lv_anim_t anim;

    if((page == NULL) || (page->menu_preview == NULL)) return;

    page->transition_active = true;
    lv_anim_init(&anim);
    lv_anim_set_var(&anim, page->menu_preview);
    lv_anim_set_values(&anim, lv_obj_get_x(page->menu_preview), target_x);
    lv_anim_set_duration(&anim, 120);
    lv_anim_set_exec_cb(&anim, (lv_anim_exec_xcb_t)lv_obj_set_x);
    lv_anim_set_user_data(&anim, page);
    lv_anim_set_completed_cb(&anim, completed_cb);
    lv_anim_start(&anim);
}

static void home_menu_drag_finish(home_page_t * page, bool commit)
{
    if((page == NULL) || (page->menu_preview == NULL) || page->transition_active) return;

    if(commit) {
        home_menu_preview_anim_start(page, 0, home_menu_anim_ready_cb);
    } else {
        home_menu_preview_anim_start(page, HOME_SCREEN_W, home_menu_cancel_anim_ready_cb);
    }
}

static void home_dropdown_drag_finish(home_page_t * page, bool commit)
{
    if((page == NULL) || (page->dropdown_preview == NULL) || page->transition_active) return;

    if(commit) {
        home_dropdown_preview_anim_start(page, 0, home_dropdown_anim_ready_cb);
    } else {
        home_dropdown_preview_anim_start(page, -132, home_dropdown_cancel_anim_ready_cb);
    }
}

static void home_menu_drag_poll(home_page_t * page, lv_indev_t * indev)
{
    lv_point_t point;
    int32_t drag_x;
    int32_t drag_y;
    bool pressed;

    if((page == NULL) || (page->root == NULL) || (indev == NULL)) return;
    if(lv_screen_active() != page->root) return;
    if(page->transition_active) return;

    pressed = (lv_indev_get_state(indev) == LV_INDEV_STATE_PRESSED);
    lv_indev_get_point(indev, &point);

    if(pressed && !page->touch_pressed) {
        page->press_point = point;
        page->drag_x = 0;
        page->drag_y = 0;
        page->drag_active = false;
        page->dropdown_drag_active = false;
        page->touch_pressed = true;
        page->release_count = 0U;
        home_page_reset_root_pos(page);
        home_menu_preview_create(page);
        home_menu_preview_hide(page);
        home_dropdown_preview_create(page);
        home_dropdown_preview_hide(page);
        return;
    }

    if(pressed) {
        page->release_count = 0U;
        drag_x = page->press_point.x - point.x;
        drag_y = point.y - page->press_point.y;

        if(!page->dropdown_drag_active &&
           !page->drag_active &&
           (page->press_point.y <= HOME_DROPDOWN_EDGE_LIMIT_Y) &&
           ((drag_y >= HOME_DROPDOWN_DRAG_START_Y) || page->dropdown_drag_active)) {
            page->dropdown_drag_active = true;
        }

        if(page->dropdown_drag_active) {
            home_dropdown_preview_set_drag(page, drag_y);
            return;
        }

        if((drag_x >= HOME_MENU_DRAG_START_X) || page->drag_active) {
            page->drag_active = true;
            home_menu_preview_set_drag(page, drag_x);
        }
        return;
    }

    if(page->touch_pressed) {
        if(page->release_count < HOME_MENU_RELEASE_CONFIRM_COUNT) {
            page->release_count++;
            return;
        }

        page->touch_pressed = false;
        page->release_count = 0U;
        if(page->dropdown_drag_active) {
            home_dropdown_drag_finish(page, page->drag_y >= HOME_DROPDOWN_DRAG_COMMIT_Y);
        } else if(page->drag_active) {
            home_menu_drag_finish(page, page->drag_x >= HOME_MENU_DRAG_COMMIT_X);
        } else {
            home_page_reset_root_pos(page);
            home_menu_preview_hide(page);
            home_dropdown_preview_hide(page);
        }
    }
}

static void home_menu_drag_timer_cb(lv_timer_t * timer)
{
    home_page_t * page = (home_page_t *)lv_timer_get_user_data(timer);
    lv_indev_t * indev = home_pointer_indev_get();

    home_menu_drag_poll(page, indev);
}

static void home_battery_update(home_page_t * page)
{
    uint8_t percent = 0U;
    int32_t level_w;

    if((page == NULL) || (page->battery_level == NULL) || (page->battery_label == NULL)) {
        return;
    }

    if((HwAccess.power.is_battery_valid != NULL) &&
       (HwAccess.power.get_battery_percent != NULL) &&
       (HwAccess.power.is_battery_valid() != 0U)) {
        percent = HwAccess.power.get_battery_percent();
        if(percent > 100U) {
            percent = 100U;
        }
        lv_label_set_text_fmt(page->battery_label, "%u%%", percent);
    } else {
        lv_label_set_text(page->battery_label, "--%");
    }

    // 电量条只映射缓存百分比，真实 ADC 采样由 Sensor_Task 周期刷新。
    level_w = (int32_t)(((uint32_t)percent * 22U) / 100U);
    lv_obj_set_width(page->battery_level, level_w);
}

static void home_battery_timer_cb(lv_timer_t * timer)
{
    home_page_t * page = (home_page_t *)lv_timer_get_user_data(timer);

    home_battery_update(page);
    home_steps_update(page);
    home_sensor_update(page);
}

/**
 * @brief 从 HwAccess 缓存刷新主页步数圆环和数值。
 */
static void home_steps_update(home_page_t * page)
{
    uint32_t steps = 0U;
    uint32_t display_steps;

    if((page == NULL) || (page->step_arc == NULL) || (page->step_label == NULL)) {
        return;
    }

    if(HwAccess.mpu6050.get_step_count != NULL) {
        steps = HwAccess.mpu6050.get_step_count();
    }

    display_steps = (steps > HOME_STEP_TARGET) ? HOME_STEP_TARGET : steps;
    lv_arc_set_range(page->step_arc, 0, HOME_STEP_TARGET);
    lv_arc_set_value(page->step_arc, display_steps);
    lv_label_set_text_fmt(page->step_label, "%lu", (unsigned long)steps);
}

/**
 * @brief 从 HwAccess 缓存刷新主页温湿度卡片。
 *
 * AHT21 的实际 I2C 测量由 Sensor_Task 周期执行；这里仅在 LVGL 线程读取缓存并更新 label。
 */
static void home_sensor_update(home_page_t * page)
{
    int16_t temp_x10;
    uint16_t humidity_x10;
    int16_t temp_c;
    uint16_t humidity_percent;

    if((page == NULL) || (page->temperature_label == NULL) || (page->humidity_label == NULL)) {
        return;
    }

    if((HwAccess.aht21.is_valid != NULL) &&
       (HwAccess.aht21.get_temperature_x10_c != NULL) &&
       (HwAccess.aht21.get_humidity_x10_percent != NULL) &&
       (HwAccess.aht21.is_valid() != 0U)) {
        temp_x10 = HwAccess.aht21.get_temperature_x10_c();
        humidity_x10 = HwAccess.aht21.get_humidity_x10_percent();

        // 卡片宽度较小，主页显示四舍五入后的整数值，精度数据仍保留在 HwAccess 缓存中。
        if(temp_x10 >= 0) {
            temp_c = (int16_t)((temp_x10 + 5) / 10);
        } else {
            temp_c = (int16_t)((temp_x10 - 5) / 10);
        }
        humidity_percent = (uint16_t)((humidity_x10 + 5U) / 10U);
        if(humidity_percent > 100U) {
            humidity_percent = 100U;
        }

        lv_label_set_text_fmt(page->temperature_label, "%d" "\xE2" "\x84" "\x83", temp_c);
        lv_label_set_text_fmt(page->humidity_label, "%u%%", humidity_percent);
    } else {
        lv_label_set_text(page->temperature_label, "--" "\xE2" "\x84" "\x83");
        lv_label_set_text(page->humidity_label, "--%");
    }
}

static lv_obj_t * home_label_create(lv_obj_t * parent, const char * text,
                                    int32_t x, int32_t y, int32_t w, int32_t h,
                                    const lv_font_t * font, lv_color_t color,
                                    lv_text_align_t align)
{
    lv_obj_t * label = lv_label_create(parent);

    lv_obj_set_pos(label, x, y);
    lv_obj_set_size(label, w, h);
    lv_label_set_text(label, text);
    lv_label_set_long_mode(label, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_color(label, color, 0);
    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_set_style_text_align(label, align, 0);
    lv_obj_set_style_text_letter_space(label, 0, 0);
    return label;
}

static void home_gesture_target(lv_obj_t * obj)
{
    lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
}

static void home_top_bar_create(home_page_t * page)
{
    // 顶部时间：使用新增 19 号字库直接绘制，不再做 transform 缩放。
    (void)home_label_create(page->root, "10:08", 16, 8 + HOME_UI_Y_OFFSET, 72, 32,
                            &my_font_source_han_19, lv_color_hex(HOME_COLOR_WHITE),
                            LV_TEXT_ALIGN_LEFT);

    // AM 小字：x=88, y=20, w=28, h=16。
    (void)home_label_create(page->root, "AM", 65, 15 + HOME_UI_Y_OFFSET, 28, 16,
                            &my_font_source_han_13, lv_color_hex(HOME_COLOR_TEXT_GRAY),
                            LV_TEXT_ALIGN_LEFT);

    lv_obj_t * battery = lv_obj_create(page->root);
    lv_obj_remove_style_all(battery);
    // 电池外框：x=155, y=15, w=30, h=16。
    lv_obj_set_pos(battery, 150, 8 + HOME_UI_Y_OFFSET);
    lv_obj_set_size(battery, 28, 15);
    lv_obj_set_style_radius(battery, 3, 0);
    lv_obj_set_style_border_width(battery, 2, 0);
    lv_obj_set_style_border_color(battery, lv_color_hex(HOME_COLOR_WHITE), 0);
    lv_obj_set_style_bg_opa(battery, LV_OPA_TRANSP, 0);
    lv_obj_clear_flag(battery, LV_OBJ_FLAG_SCROLLABLE);

    // lv_obj_t * cap = lv_obj_create(page->root);
    // lv_obj_remove_style_all(cap);
    // lv_obj_set_pos(cap, 187, 18);
    // lv_obj_set_size(cap, 4, 7);
    // lv_obj_set_style_radius(cap, 2, 0);
    // lv_obj_set_style_bg_color(cap, lv_color_hex(HOME_COLOR_WHITE), 0);
    // lv_obj_set_style_bg_opa(cap, LV_OPA_COVER, 0);

    page->battery_level = lv_obj_create(battery);
    lv_obj_remove_style_all(page->battery_level);
    // 电池内部绿色电量条宽度由缓存百分比刷新，避免主页直接触发 ADC 采样。
    lv_obj_set_pos(page->battery_level, 1, 1);
    lv_obj_set_size(page->battery_level, 0, 9);
    lv_obj_set_style_radius(page->battery_level, 2, 0);
    lv_obj_set_style_bg_color(page->battery_level, lv_color_hex(HOME_COLOR_GREEN), 0);
    lv_obj_set_style_bg_opa(page->battery_level, LV_OPA_COVER, 0);

    // 电量文字：使用新增 18 号字库直接绘制。
    page->battery_label = home_label_create(page->root, "--%", 180, 6 + HOME_UI_Y_OFFSET, 44, 30,
                                            &my_font_source_han_18,
                                            lv_color_hex(HOME_COLOR_WHITE),
                                            LV_TEXT_ALIGN_LEFT);
    home_battery_update(page);
}

static void home_steps_panel_create(home_page_t * page)
{
    const int32_t center_x = HOME_SCREEN_W / 2;
    const int32_t arc_size = 170;
    const int32_t arc_y = 44 + HOME_UI_Y_OFFSET;
    lv_obj_t * panel = lv_obj_create(page->root);
    lv_obj_remove_style_all(panel);
    // 步数圆环区域：x=44, y=36, w=152, h=152，圆心自然落在 (120,112)。
    lv_obj_set_pos(panel, 44, 36 + HOME_UI_Y_OFFSET);
    lv_obj_set_size(panel, arc_size, arc_size);
    lv_obj_set_pos(panel, center_x - (arc_size / 2), arc_y);
    lv_obj_set_style_bg_opa(panel, LV_OPA_TRANSP, 0);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);

    page->step_arc = lv_arc_create(panel);
    lv_obj_remove_style(page->step_arc, NULL, LV_PART_KNOB);
    lv_obj_set_pos(page->step_arc, 0, 0);
    lv_obj_set_size(page->step_arc, arc_size, arc_size);
    // 圆环角度：背景从 130 到 410；进度到 360 约为 82%。
    lv_arc_set_bg_angles(page->step_arc, 130, 410);
    lv_arc_set_angles(page->step_arc, 130, 360);
    lv_arc_set_range(page->step_arc, 0, HOME_STEP_TARGET);
    lv_arc_set_value(page->step_arc, 0);
    lv_obj_clear_flag(page->step_arc, LV_OBJ_FLAG_CLICKABLE);
    // 圆环线宽：按需求固定 8px。
    lv_obj_set_style_arc_width(page->step_arc, 8, LV_PART_MAIN);
    lv_obj_set_style_arc_width(page->step_arc, 8, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(page->step_arc, lv_color_hex(HOME_COLOR_ARC_BG), LV_PART_MAIN);
    lv_obj_set_style_arc_opa(page->step_arc, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_arc_color(page->step_arc, lv_color_hex(HOME_COLOR_GREEN), LV_PART_INDICATOR);

    // 步数图标：x=104, y=58, w=32, h=32；使用 iconfont 的 U+E646 运动步数图标。
    // 图标按 iconfont 原始尺寸绘制，不再做 transform 缩放。
    (void)home_label_create(page->root, MY_ICON_SHOE, 103, 62 + HOME_UI_Y_OFFSET, 32, 32,
                            &my_icon_fonts, lv_color_hex(HOME_COLOR_GREEN),
                            LV_TEXT_ALIGN_CENTER);

    // STEPS 标签：使用新增 13 号字库直接绘制。
    (void)home_label_create(page->root, "STEPS", 80, 92 + HOME_UI_Y_OFFSET, 80, 24,
                            &my_font_source_han_13, lv_color_hex(HOME_COLOR_GREEN),
                            LV_TEXT_ALIGN_CENTER);

    // 步数数值：使用新增 38 号字库直接绘制。
    page->step_label = home_label_create(page->root, "0", 35, 115 + HOME_UI_Y_OFFSET, 170, 58,
                                         &my_font_source_han_38,
                                         lv_color_hex(HOME_COLOR_WHITE),
                                         LV_TEXT_ALIGN_CENTER);

    // 目标步数：使用新增 18 号字库直接绘制。
    (void)home_label_create(page->root, "/ 15,000", 76, 165 + HOME_UI_Y_OFFSET, 88, 24,
                            &my_font_source_han_18, lv_color_hex(HOME_COLOR_TEXT_GRAY),
                            LV_TEXT_ALIGN_CENTER);
    home_steps_update(page);

    // 这四个 label 刚创建完成后是 root 的最后四个子对象，统一放入同一条中轴盒子。
    // uint32_t child_count = lv_obj_get_child_count(page->root);
    // if(child_count >= 4U) {
    //     lv_obj_t * shoe_label = lv_obj_get_child(page->root, child_count - 4U);
    //     lv_obj_t * steps_label = lv_obj_get_child(page->root, child_count - 3U);
    //     lv_obj_t * value_label = lv_obj_get_child(page->root, child_count - 2U);
    //     lv_obj_t * target_label = lv_obj_get_child(page->root, child_count - 1U);

        // lv_obj_set_pos(shoe_label, content_x, arc_y + 18);
        // lv_obj_set_size(shoe_label, content_w, 32);
        // lv_obj_set_pos(steps_label, content_x, arc_y + 50);
        // lv_obj_set_size(steps_label, content_w, 24);
        // lv_obj_set_pos(value_label, content_x, arc_y + 78);
        // lv_obj_set_size(value_label, content_w, 58);
        // lv_obj_set_pos(target_label, content_x, arc_y + 123);
        // lv_obj_set_size(target_label, content_w, 24);
    // }
}

static void home_sun_icon_create(lv_obj_t * parent, int32_t x, int32_t y)
{
    (void)home_label_create(parent, MY_ICON_SUN, x, y, 24, 24,
                            &my_icon_fonts, lv_color_hex(HOME_COLOR_ORANGE),
                            LV_TEXT_ALIGN_CENTER);
}

static lv_obj_t * home_metric_card_create(lv_obj_t * parent, int32_t x, int32_t y,
                                          uint32_t border_hex, const char * symbol,
                                          uint32_t symbol_hex, const char * value,
                                          const char * title)
{
    lv_obj_t * card = lv_obj_create(parent);
    lv_obj_t * value_label;

    lv_obj_remove_style_all(card);
    // 底部卡片：左卡传入 x=18，右卡传入 x=126；统一 w=96, h=58, radius=14。
    lv_obj_set_pos(card, x, y);
    // 保留原有卡片 4px 对齐修正，再跟随主页整体下移 4px。
    lv_obj_set_y(card, y + 4 + HOME_UI_Y_OFFSET);
    lv_obj_set_size(card, 96, 56);
    lv_obj_set_style_radius(card, 14, 0);
    lv_obj_set_style_bg_color(card, lv_color_hex(HOME_COLOR_PANEL), 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(border_hex), 0);
    lv_obj_set_style_pad_all(card, 0, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    if((symbol != NULL) && (strcmp(symbol, LV_SYMBOL_TINT) == 0)) {
        symbol = MY_ICON_WATER_DROP;
    }

    if(symbol == NULL) {
        // 温度卡片图标位置：卡片内 x=10, y=13，对应屏幕绝对坐标 x=28, y=211。
        home_sun_icon_create(card, 10, 13);
    } else {
        // 湿度卡片图标位置：卡片内 x=10, y=13，对应屏幕绝对坐标 x=136, y=211。
        (void)home_label_create(card, symbol, 10, 13, 24, 24,
                                &my_icon_fonts, lv_color_hex(symbol_hex),
                                LV_TEXT_ALIGN_CENTER);
    }

    // 卡片数值：使用新增 19 号字库直接绘制。
    value_label = home_label_create(card, value, 40, 7, 48, 30,
                                    &my_font_source_han_19, lv_color_hex(HOME_COLOR_WHITE),
                                    LV_TEXT_ALIGN_LEFT);

    // 卡片底部标签：卡片内 x=10, y=38, w=76, h=14。
    (void)home_label_create(card, title, 10, 38, 76, 14,
                            &my_font_source_han_10, lv_color_hex(symbol_hex),
                            LV_TEXT_ALIGN_CENTER);

    return value_label;
}

home_page_t * home_page_create(void)
{
    static home_page_t page_storage;
    home_page_t * page = &page_storage;
    memset(page, 0, sizeof(*page));

    page->root = lv_obj_create(NULL);
    lv_obj_remove_style_all(page->root);
    lv_obj_set_size(page->root, HOME_SCREEN_W, HOME_SCREEN_H);
    lv_obj_set_style_bg_color(page->root, lv_color_hex(HOME_COLOR_BG), 0);
    lv_obj_set_style_bg_opa(page->root, LV_OPA_COVER, 0);
    lv_obj_set_scrollbar_mode(page->root, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(page->root, LV_OBJ_FLAG_SCROLLABLE);
    home_gesture_target(page->root);
    lv_obj_add_event_cb(page->root, home_page_key_cb, LV_EVENT_KEY, NULL);

    home_top_bar_create(page);
    home_steps_panel_create(page);
    // 左侧温度卡片：屏幕 x=18, y=198, w=96, h=58。
    page->temperature_label = home_metric_card_create(page->root, 18, 198, 0x225b75, NULL,
                                                      HOME_COLOR_ORANGE,
                                                      "--" "\xE2" "\x84" "\x83",
                                                      "TEMP");
    // 右侧湿度卡片：屏幕 x=126, y=198, w=96, h=58；两卡间距 12px。
    page->humidity_label = home_metric_card_create(page->root, 126, 198, 0x1c4b98, LV_SYMBOL_TINT,
                                                   HOME_COLOR_BLUE, "--%", "HUMID");
    home_sensor_update(page);
    home_menu_preview_create(page);
    home_dropdown_preview_create(page);

    page->drag_timer = lv_timer_create(home_menu_drag_timer_cb,
                                       HOME_MENU_DRAG_TIMER_MS,
                                       page);
    page->battery_timer = lv_timer_create(home_battery_timer_cb,
                                          HOME_BATTERY_REFRESH_MS,
                                          page);

    return page;
}

void home_page_destroy(home_page_t * page)
{
    if(!page) return;
    if(page->drag_timer) {
        lv_timer_del(page->drag_timer);
        page->drag_timer = NULL;
    }
    if(page->battery_timer) {
        lv_timer_del(page->battery_timer);
        page->battery_timer = NULL;
    }
    home_menu_preview_delete(page);
    home_dropdown_preview_delete(page);
    if(page->root) lv_obj_del(page->root);
    memset(page, 0, sizeof(*page));
}

lv_obj_t * home_page_root(home_page_t * page)
{
    return page ? page->root : NULL;
}

/**
 * @brief 创建并加载默认主页。
 */
static void HomePage_Create(void)
{
    lv_obj_t * root;

    if(s_home_page == NULL) {
        s_home_page = home_page_create();
        if(s_home_page == NULL) {
            return;
        }
    }

    root = home_page_root(s_home_page);
    if(root == NULL) {
        home_page_destroy(s_home_page);
        s_home_page = NULL;
        return;
    }

    lv_screen_load(root);
    lv_obj_invalidate(root);
}

/**
 * @brief 释放默认主页。
 */
static void HomePage_Destroy(void)
{
    home_page_destroy(s_home_page);
    s_home_page = NULL;
}

const GUI_Page_t HomePage = {
    .create = HomePage_Create,
    .destroy = HomePage_Destroy,
};
