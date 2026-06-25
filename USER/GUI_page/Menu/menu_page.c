/**
 * @file menu_page.c
 * @brief 主菜单页面实现。
 *
 * 本页面负责显示应用入口列表，并通过 PageManager 切换到对应功能页面。
 */

#include "menu_page.h"

#include <string.h>

#include "animation_page.h"
#include "calendar_page.h"
#include "calculator_page.h"
#include "heart_rate_page.h"
#include "stopwatch_page.h"
#include "test_page.h"
#include "title_page.h"

/**
 * @brief 主菜单页面对象。
 *
 * root 是页面根 screen，list 是 root 的子对象。last_clicked_btn 指向 list 中的按钮，
 * 当 root 被删除后该指针失效。
 */
struct menu_page {
    lv_obj_t * root;               /**< 页面根 screen，归本页面对象所有。 */
    lv_obj_t * list;               /**< 菜单列表容器，root 删除时自动删除。 */
    lv_timer_t * sb_hide_timer;    /**< 滚动条延迟隐藏定时器，由本页面删除。 */
    lv_obj_t * last_clicked_btn;   /**< 最近点击的菜单按钮，仅在本页面存活时有效。 */
    int32_t last_scroll_y;         /**< 进入子页面前记录的列表滚动位置。 */
};

extern const lv_font_t my_font_source_han_20;

#define MENU_ITEM_COUNT 13

typedef struct {
    const char * title;
    const char * icon;
    uint32_t icon_bg_hex;
} menu_item_desc_t;

typedef struct {
    struct menu_page * owner;      /**< 所属菜单页面，不负责释放。 */
    const char * title;            /**< 菜单标题，指向静态字符串。 */
    lv_obj_t * icon_bg;            /**< 图标背景对象，随按钮删除。 */
    lv_obj_t * title_label;        /**< 标题 label，随按钮删除。 */
    lv_indev_t * pressed_indev;    /**< 当前按下的输入设备，不负责释放。 */
    lv_timer_t * press_timer;      /**< 延迟高亮定时器，由事件回调删除。 */
    bool effect_on;                /**< 标记当前高亮效果是否已开启。 */
} menu_item_ctx_t;

static menu_item_ctx_t s_menu_item_ctx[MENU_ITEM_COUNT];
static menu_page_t * s_menu_page;
static int32_t s_menu_saved_scroll_y;
static bool s_menu_saved_scroll_valid;
static const char * s_menu_pending_title;

/**
 * @brief 在 LVGL 事件回调返回后再执行页面跳转。
 *
 * 触摸点击事件处理中直接销毁当前菜单 screen，会让 LVGL 输入设备仍持有刚被删除的
 * 按钮/列表状态。这里使用 lv_async_call() 延后到当前事件栈结束后再切页，避免触摸释放
 * 与页面销毁交织在一起。
 */
static void menu_page_async_push_cb(void * user_data)
{
    const GUI_Page_t * page = (const GUI_Page_t *)user_data;

    if(page == NULL) {
        return;
    }

    if(page == &TitlePage) {
        title_page_set_title(s_menu_pending_title);
    }

    s_menu_pending_title = NULL;
    (void)PageManager_Push(page);
}

static void menu_page_request_push(const GUI_Page_t * page, const char * title)
{
    if(page == NULL) {
        return;
    }

    s_menu_pending_title = title;

    if(lv_async_call(menu_page_async_push_cb, (void *)page) != LV_RESULT_OK) {
        s_menu_pending_title = NULL;
        if(page == &TitlePage) {
            title_page_set_title(title);
        }
        (void)PageManager_Push(page);
    }
}

static void menu_item_effect_set(menu_item_ctx_t * ctx, bool on)
{
    if(!ctx) return;
    if(ctx->effect_on == on) return;
    ctx->effect_on = on;

    if(ctx->icon_bg) {
        if(on) lv_obj_add_state(ctx->icon_bg, LV_STATE_USER_1);
        else lv_obj_clear_state(ctx->icon_bg, LV_STATE_USER_1);
    }
    if(ctx->title_label) {
        if(on) lv_obj_add_state(ctx->title_label, LV_STATE_USER_1);
        else lv_obj_clear_state(ctx->title_label, LV_STATE_USER_1);
    }
}

static void menu_item_press_timer_cb(lv_timer_t * t)
{
    menu_item_ctx_t * ctx = (menu_item_ctx_t *)lv_timer_get_user_data(t);
    if(!ctx) return;

    ctx->press_timer = NULL;
    lv_timer_del(t);

    menu_item_effect_set(ctx, true);
}

static void menu_item_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    menu_item_ctx_t * ctx = (menu_item_ctx_t *)lv_event_get_user_data(e);

    if(code == LV_EVENT_PRESSED) {
        if(!ctx) return;
        ctx->pressed_indev = lv_event_get_indev(e);

        if(ctx->press_timer) lv_timer_del(ctx->press_timer);
        ctx->press_timer = lv_timer_create(menu_item_press_timer_cb, 120, ctx);
        lv_timer_set_repeat_count(ctx->press_timer, 1);
    }
    else if(code == LV_EVENT_PRESS_LOST) {
        // 滑动列表时 LVGL 会让按钮失去 press 状态，但手指还没松开，保留高亮反馈。
    }
    else if(code == LV_EVENT_RELEASED) {
        if(ctx && ctx->press_timer) {
            lv_timer_del(ctx->press_timer);
            ctx->press_timer = NULL;
        }
        if(ctx) {
            ctx->pressed_indev = NULL;
            menu_item_effect_set(ctx, false);
        }
    }
    else if(code == LV_EVENT_CLICKED) {
        const char * title = ctx ? ctx->title : NULL;

        if(ctx && ctx->owner) {
            ctx->owner->last_clicked_btn = lv_event_get_target(e);
            if(ctx->owner->list) {
                ctx->owner->last_scroll_y = lv_obj_get_scroll_y(ctx->owner->list);
                s_menu_saved_scroll_y = ctx->owner->last_scroll_y;
                s_menu_saved_scroll_valid = true;
            }
        }

        if(title == NULL) {
            return;
        }

        if(strcmp(title, "日历") == 0) {
            menu_page_request_push(&CalendarPage, NULL);
        }
        else if(strcmp(title, "计算器") == 0) {
            menu_page_request_push(&CalculatorPage, NULL);
        }
        else if(strcmp(title, "秒表") == 0) {
            menu_page_request_push(&StopwatchPage, NULL);
        }
        else if(strcmp(title, "动画") == 0) {
            menu_page_request_push(&AnimationPage, NULL);
        }
        else if(strcmp(title, "心率") == 0) {
            menu_page_request_push(&HeartRatePage, NULL);
        }
        else if(strcmp(title, "测试") == 0) {
            menu_page_request_push(&TestPage, NULL);
        }
        else {
            menu_page_request_push(&TitlePage, title);
        }
    }
    else if(code == LV_EVENT_DELETE) {
        if(ctx && ctx->press_timer) {
            lv_timer_del(ctx->press_timer);
            ctx->press_timer = NULL;
        }
    }
}

static lv_obj_t * menu_item_create(lv_obj_t * parent, menu_page_t * owner,
                                   const menu_item_desc_t * desc, menu_item_ctx_t * ctx)
{
    lv_obj_t * btn = lv_btn_create(parent);
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_CLICK_FOCUSABLE);
    lv_obj_set_width(btn, lv_pct(100));
    lv_obj_set_height(btn, 70);
    lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(btn, 0, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_set_style_radius(btn, 0, 0);
    lv_obj_set_style_pad_hor(btn, 18, 0);
    lv_obj_set_style_pad_ver(btn, 10, 0);

    lv_obj_set_style_bg_color(btn, lv_color_hex(0x192027), LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(btn, LV_OPA_60, LV_STATE_PRESSED);
    lv_obj_set_style_translate_y(btn, 2, LV_STATE_PRESSED);

    if(ctx) {
        memset(ctx, 0, sizeof(*ctx));
        ctx->owner = owner;
        ctx->title = desc->title;
    }

    lv_obj_add_event_cb(btn, menu_item_event_cb, LV_EVENT_ALL, ctx);

    lv_obj_t * icon_bg = lv_obj_create(btn);
    lv_obj_clear_flag(icon_bg, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(icon_bg, 46, 46);
    lv_obj_set_style_radius(icon_bg, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(icon_bg, lv_color_hex(desc->icon_bg_hex), 0);
    lv_obj_set_style_clip_corner(icon_bg, true, 0);
    lv_obj_set_style_border_width(icon_bg, 0, 0);
    lv_obj_set_style_pad_all(icon_bg, 0, 0);
    lv_obj_align(icon_bg, LV_ALIGN_LEFT_MID, 0, 0);

    lv_obj_t * icon = lv_label_create(icon_bg);
    lv_label_set_text(icon, desc->icon);
    lv_obj_set_style_text_color(icon, lv_color_white(), 0);
    lv_obj_center(icon);

    lv_obj_t * title = lv_label_create(btn);
    lv_label_set_text(title, desc->title);
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_set_style_text_font(title, &my_font_source_han_20, 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 62, 0);

    if(ctx) {
        ctx->icon_bg = icon_bg;
        ctx->title_label = title;
    }

    lv_obj_set_style_bg_opa(icon_bg, LV_OPA_70, LV_STATE_USER_1);
    lv_obj_set_style_transform_zoom(icon_bg, 230, LV_STATE_USER_1);

    lv_obj_set_style_text_color(title, lv_color_hex(0xe9ecef), LV_STATE_USER_1);

    return btn;
}

static void menu_page_key_cb(lv_event_t * e)
{
    if(lv_event_get_code(e) != LV_EVENT_KEY) return;
    if(lv_event_get_key(e) != LV_KEY_ESC) return;
    (void)PageManager_Pop();
}

static void menu_page_restore_scroll(menu_page_t * page)
{
    if(!page || !page->list) return;

    lv_obj_update_layout(page->list);
    lv_obj_scroll_to_y(page->list, page->last_scroll_y, LV_ANIM_OFF);
}

static void sb_hide_timer_cb(lv_timer_t * t)
{
    menu_page_t * page = (menu_page_t *)lv_timer_get_user_data(t);
    if(!page || !page->list) return;

    lv_obj_set_scrollbar_mode(page->list, LV_SCROLLBAR_MODE_ACTIVE);
    page->sb_hide_timer = NULL;
    lv_timer_del(t);
}

static void list_scroll_event_cb(lv_event_t * e)
{
    menu_page_t * page = (menu_page_t *)lv_event_get_user_data(e);
    if(!page || !page->list) return;

    lv_event_code_t code = lv_event_get_code(e);
    if(code != LV_EVENT_SCROLL && code != LV_EVENT_SCROLL_BEGIN && code != LV_EVENT_SCROLL_END) return;

    lv_obj_set_scrollbar_mode(page->list, LV_SCROLLBAR_MODE_ON);

    if(page->sb_hide_timer) {
        lv_timer_reset(page->sb_hide_timer);
    }
    else {
        page->sb_hide_timer = lv_timer_create(sb_hide_timer_cb, 1500, page);
        lv_timer_set_repeat_count(page->sb_hide_timer, 1);
    }
}

menu_page_t * menu_page_create(void)
{
    static const menu_item_desc_t items[] = {
        { "日历", LV_SYMBOL_LIST, 0xff6b6b },
        { "计算器", LV_SYMBOL_SETTINGS, 0xffa94d },
        { "秒表", LV_SYMBOL_REFRESH, 0xb197fc },
        { "动画", LV_SYMBOL_PLAY, 0x4dabf7 },
        { "卡包", LV_SYMBOL_DIRECTORY, 0xffd43b },
        { "运动", LV_SYMBOL_UP, 0x51cf66 },
        { "心率", LV_SYMBOL_TINT, 0xff8787 },
        { "血氧", LV_SYMBOL_AUDIO, 0x74c0fc },
        { "环境", LV_SYMBOL_HOME, 0x63e6be },
        { "测试", LV_SYMBOL_EYE_OPEN, 0x66d9e8 },
        { "游戏", LV_SYMBOL_PLAY, 0xffc078 },
        { "设置", LV_SYMBOL_SETTINGS, 0xadb5bd },
        { "关于", LV_SYMBOL_COPY, 0xdee2e6 },
    };

    static menu_page_t page_storage;
    menu_page_t * page = &page_storage;
    memset(page, 0, sizeof(*page));
    memset(s_menu_item_ctx, 0, sizeof(s_menu_item_ctx));

    page->root = lv_obj_create(NULL);
    lv_obj_remove_style_all(page->root);
    lv_obj_set_size(page->root, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(page->root, lv_color_hex(0x0b0f12), 0);
    lv_obj_set_style_bg_opa(page->root, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(page->root, 0, 0);
    lv_obj_set_style_pad_all(page->root, 8, 0);
    lv_obj_set_scrollbar_mode(page->root, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_event_cb(page->root, menu_page_key_cb, LV_EVENT_KEY, NULL);

    page->list = lv_obj_create(page->root);
    lv_obj_remove_style_all(page->list);
    page->sb_hide_timer = NULL;
    page->last_clicked_btn = NULL;
    page->last_scroll_y = s_menu_saved_scroll_valid ? s_menu_saved_scroll_y : 0;

    lv_obj_set_size(page->list, LV_HOR_RES - 16, LV_VER_RES - 16);
    lv_obj_set_style_bg_opa(page->list, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(page->list, 0, 0);
    lv_obj_set_style_pad_all(page->list, 0, 0);
    lv_obj_set_style_pad_row(page->list, 8, 0);
    lv_obj_set_flex_flow(page->list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(page->list, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_scroll_dir(page->list, LV_DIR_VER);
    lv_obj_add_flag(page->list, LV_OBJ_FLAG_SCROLL_MOMENTUM);
    lv_obj_set_scrollbar_mode(page->list, LV_SCROLLBAR_MODE_ACTIVE);
    lv_obj_set_style_width(page->list, 6, LV_PART_SCROLLBAR);
    lv_obj_set_style_pad_right(page->list, 2, LV_PART_SCROLLBAR);
    lv_obj_set_style_radius(page->list, LV_RADIUS_CIRCLE, LV_PART_SCROLLBAR);
    lv_obj_set_style_bg_color(page->list, lv_color_white(), LV_PART_SCROLLBAR);
    lv_obj_set_style_bg_opa(page->list, LV_OPA_40, LV_PART_SCROLLBAR);
    lv_obj_add_event_cb(page->list, list_scroll_event_cb, LV_EVENT_ALL, page);
    lv_obj_center(page->list);

    for(size_t i = 0; i < (sizeof(items) / sizeof(items[0])); i++) {
        (void)menu_item_create(page->list, page, &items[i], &s_menu_item_ctx[i]);
    }

    menu_page_restore_scroll(page);

    return page;
}

void menu_page_destroy(menu_page_t * page)
{
    if(!page) return;
    if(page->sb_hide_timer) {
        lv_timer_del(page->sb_hide_timer);
        page->sb_hide_timer = NULL;
    }
    if(page->root) lv_obj_del(page->root);
    memset(page, 0, sizeof(*page));
    memset(s_menu_item_ctx, 0, sizeof(s_menu_item_ctx));
}

lv_obj_t * menu_page_root(menu_page_t * page)
{
    return page ? page->root : NULL;
}

/**
 * @brief 创建并加载主菜单页面。
 */
static void MenuPage_Create(void)
{
    lv_obj_t * root;

    if(s_menu_page == NULL) {
        s_menu_page = menu_page_create();
        if(s_menu_page == NULL) {
            return;
        }
    }

    root = menu_page_root(s_menu_page);
    if(root == NULL) {
        menu_page_destroy(s_menu_page);
        s_menu_page = NULL;
        return;
    }

    lv_screen_load(root);
    lv_obj_invalidate(root);
}

/**
 * @brief 释放主菜单页面。
 */
static void MenuPage_Destroy(void)
{
    menu_page_destroy(s_menu_page);
    s_menu_page = NULL;
}

const GUI_Page_t MenuPage = {
    .create = MenuPage_Create,
    .destroy = MenuPage_Destroy,
};
