/**
 * @file dropdown_menu_page.c
 * @brief 涓嬫粦蹇嵎鑿滃崟椤甸潰瀹炵幇銆?
 *
 * 鏈〉闈㈠彧鍦?GUI 浠诲姟涓婁笅鏂囦腑閫氳繃 HwAccess 鎺у埗钃濈墮寮€鍏筹紝涓嶇洿鎺ヨ闂?BSP/HAL銆?
 */

#include "dropdown_menu_page.h"

#include <string.h>

#include "hwaccess.h"

extern const lv_font_t my_font_source_han_20;

struct dropdown_menu_page {
    lv_obj_t * root;
    lv_obj_t * title_label;
    lv_obj_t * panel;
    lv_obj_t * item_label;
    lv_obj_t * state_label;
    lv_obj_t * bluetooth_switch;
};

static dropdown_menu_page_t * s_dropdown_menu_page;

static void dropdown_menu_page_key_cb(lv_event_t * e);
static void dropdown_menu_page_apply_bluetooth_state(dropdown_menu_page_t * page,
                                                     uint8_t enabled);
static void dropdown_menu_page_panel_event_cb(lv_event_t * e);
static void dropdown_menu_page_switch_event_cb(lv_event_t * e);
static void dropdown_menu_page_sync_bluetooth(dropdown_menu_page_t * page);

/**
 * @brief 澶勭悊 ESC 杩斿洖閿€?
 */
static void dropdown_menu_page_key_cb(lv_event_t * e)
{
    if(lv_event_get_code(e) != LV_EVENT_KEY) {
        return;
    }

    if(lv_event_get_key(e) == LV_KEY_ESC) {
        (void)PageManager_Pop();
    }
}

/**
 * @brief 鏍规嵁褰撳墠钃濈墮鐘舵€佸埛鏂板紑鍏冲拰鏂囧瓧銆?
 */
static void dropdown_menu_page_sync_bluetooth(dropdown_menu_page_t * page)
{
    uint8_t enabled = 0U;

    if(page == NULL) {
        return;
    }

    if(HwAccess.bluetooth.is_enabled != NULL) {
        enabled = HwAccess.bluetooth.is_enabled();
    }

    if(page->bluetooth_switch != NULL) {
        if(enabled != 0U) {
            lv_obj_add_state(page->bluetooth_switch, LV_STATE_CHECKED);
        } else {
            lv_obj_clear_state(page->bluetooth_switch, LV_STATE_CHECKED);
        }
    }

    if(page->state_label != NULL) {
        if(enabled != 0U) {
            lv_label_set_text(page->state_label, "ON");
            lv_obj_set_style_text_color(page->state_label, lv_color_hex(0x4ad295), 0);
        } else {
            lv_label_set_text(page->state_label, "OFF");
            lv_obj_set_style_text_color(page->state_label, lv_color_hex(0xa5b1c2), 0);
        }
    }
}

/**
 * @brief 鎸夌洰鏍囩姸鎬佹墽琛屼竴娆¤摑鐗欏紑鍏虫搷浣溿€?
 */
static void dropdown_menu_page_apply_bluetooth_state(dropdown_menu_page_t * page,
                                                     uint8_t enabled)
{
    if(page == NULL) {
        return;
    }

    if(enabled != 0U) {
        if(HwAccess.bluetooth.enable != NULL) {
            HwAccess.bluetooth.enable();
        }
    } else {
        if(HwAccess.bluetooth.disable != NULL) {
            HwAccess.bluetooth.disable();
        }
    }

    dropdown_menu_page_sync_bluetooth(page);
}

/**
 * @brief 鐐瑰嚮鏁村潡鍗＄墖鏃讹紝涔熷垏鎹竴娆¤摑鐗欏紑鍏炽€?
 */
static void dropdown_menu_page_panel_event_cb(lv_event_t * e)
{
    dropdown_menu_page_t * page = (dropdown_menu_page_t *)lv_event_get_user_data(e);
    uint8_t enabled = 0U;

    if((lv_event_get_code(e) != LV_EVENT_CLICKED) || (page == NULL)) {
        return;
    }

    if(HwAccess.bluetooth.is_enabled != NULL) {
        enabled = HwAccess.bluetooth.is_enabled();
    }

    dropdown_menu_page_apply_bluetooth_state(page, (enabled == 0U) ? 1U : 0U);
}

/**
 * @brief 澶勭悊钃濈墮寮€鍏冲垏鎹簨浠躲€?
 */
static void dropdown_menu_page_switch_event_cb(lv_event_t * e)
{
    dropdown_menu_page_t * page = (dropdown_menu_page_t *)lv_event_get_user_data(e);
    lv_obj_t * sw = lv_event_get_target(e);
    uint8_t checked;

    if((lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) || (page == NULL) || (sw == NULL)) {
        return;
    }

    checked = lv_obj_has_state(sw, LV_STATE_CHECKED) ? 1U : 0U;
    dropdown_menu_page_apply_bluetooth_state(page, checked);
}

dropdown_menu_page_t * dropdown_menu_page_create(void)
{
    static dropdown_menu_page_t page_storage;
    dropdown_menu_page_t * page = &page_storage;

    memset(page, 0, sizeof(*page));

    page->root = lv_obj_create(NULL);
    if(page->root == NULL) {
        return NULL;
    }

    lv_obj_set_size(page->root, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(page->root, lv_color_hex(0x0b0f12), 0);
    lv_obj_set_style_border_width(page->root, 0, 0);
    lv_obj_set_style_pad_all(page->root, 16, 0);
    lv_obj_set_scrollbar_mode(page->root, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(page->root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(page->root, dropdown_menu_page_key_cb, LV_EVENT_KEY, NULL);

    page->title_label = lv_label_create(page->root);
    if(page->title_label != NULL) {
        lv_label_set_text(page->title_label, "蹇嵎鑿滃崟");
        lv_obj_set_style_text_font(page->title_label, &my_font_source_han_20, 0);
        lv_obj_set_style_text_color(page->title_label, lv_color_white(), 0);
        lv_obj_align(page->title_label, LV_ALIGN_TOP_LEFT, 2, 0);
    }

    page->panel = lv_obj_create(page->root);
    if(page->panel != NULL) {
        lv_obj_set_size(page->panel, lv_pct(100), 88);
        lv_obj_align(page->panel, LV_ALIGN_TOP_MID, 0, 52);
        lv_obj_set_style_bg_color(page->panel, lv_color_hex(0x15202b), 0);
        lv_obj_set_style_bg_opa(page->panel, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(page->panel, 0, 0);
        lv_obj_set_style_radius(page->panel, 18, 0);
        lv_obj_set_style_pad_hor(page->panel, 18, 0);
        lv_obj_set_style_pad_ver(page->panel, 16, 0);
        lv_obj_clear_flag(page->panel, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_event_cb(page->panel,
                            dropdown_menu_page_panel_event_cb,
                            LV_EVENT_CLICKED,
                            page);
    }

    page->item_label = lv_label_create(page->panel);
    if(page->item_label != NULL) {
        lv_label_set_text(page->item_label, "s");
        lv_obj_set_style_text_font(page->item_label, &my_font_source_han_20, 0);
        lv_obj_set_style_text_color(page->item_label, lv_color_white(), 0);
        lv_obj_align(page->item_label, LV_ALIGN_TOP_LEFT, 0, 0);
    }

    page->state_label = lv_label_create(page->panel);
    if(page->state_label != NULL) {
        lv_label_set_text(page->state_label, "--");
        lv_obj_set_style_text_font(page->state_label, &my_font_source_han_20, 0);
        lv_obj_align(page->state_label, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    }

    page->bluetooth_switch = lv_switch_create(page->panel);
    if(page->bluetooth_switch != NULL) {
        lv_obj_align(page->bluetooth_switch, LV_ALIGN_RIGHT_MID, 0, 0);
        lv_obj_add_event_cb(page->bluetooth_switch,
                            dropdown_menu_page_switch_event_cb,
                            LV_EVENT_VALUE_CHANGED,
                            page);
    }

    dropdown_menu_page_sync_bluetooth(page);
    return page;
}

void dropdown_menu_page_destroy(dropdown_menu_page_t * page)
{
    if(page == NULL) {
        return;
    }

    if(page->root != NULL) {
        lv_obj_del(page->root);
    }

    memset(page, 0, sizeof(*page));
}

lv_obj_t * dropdown_menu_page_root(dropdown_menu_page_t * page)
{
    return (page != NULL) ? page->root : NULL;
}

/**
 * @brief 鍒涘缓骞跺姞杞戒笅婊戝揩鎹疯彍鍗曢〉闈€?
 */
static void DropdownMenuPage_Create(void)
{
    lv_obj_t * root;

    if(s_dropdown_menu_page != NULL) {
        dropdown_menu_page_destroy(s_dropdown_menu_page);
        s_dropdown_menu_page = NULL;
    }

    s_dropdown_menu_page = dropdown_menu_page_create();
    if(s_dropdown_menu_page == NULL) {
        return;
    }

    root = dropdown_menu_page_root(s_dropdown_menu_page);
    if(root == NULL) {
        dropdown_menu_page_destroy(s_dropdown_menu_page);
        s_dropdown_menu_page = NULL;
        return;
    }

    lv_screen_load(root);
}

/**
 * @brief 閲婃斁涓嬫粦蹇嵎鑿滃崟椤甸潰銆?
 */
static void DropdownMenuPage_Destroy(void)
{
    dropdown_menu_page_destroy(s_dropdown_menu_page);
    s_dropdown_menu_page = NULL;
}

const GUI_Page_t DropdownMenuPage = {
    .create = DropdownMenuPage_Create,
    .destroy = DropdownMenuPage_Destroy,
};
