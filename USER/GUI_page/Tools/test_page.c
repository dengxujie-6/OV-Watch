/**
 * @file test_page.c
 * @brief 系统测试页面实现。
 */

#include "test_page.h"

#include <string.h>

#include "debug_overlay.h"
#include "hwaccess.h"

#define TEST_PAGE_REFRESH_MS 500U

struct test_page {
    lv_obj_t * root;          /**< 页面根 screen，归本页面对象所有。*/
    lv_obj_t * fps_label;     /**< FPS 和 LVGL 内存状态。*/
    lv_obj_t * power_label;   /**< 电池缓存状态。*/
    lv_obj_t * mpu_label;     /**< MPU6050 缓存状态。*/
    lv_timer_t * timer;       /**< 页面刷新定时器，由本页面删除。*/
};

static test_page_t * s_test_page;

static void test_page_key_cb(lv_event_t * e);
static void test_page_timer_cb(lv_timer_t * timer);
static lv_obj_t * test_page_create_label(lv_obj_t * parent, lv_coord_t y, lv_color_t color);
static void test_page_update(test_page_t * page);

/**
 * @brief 处理返回按键。
 */
static void test_page_key_cb(lv_event_t * e)
{
    if(lv_event_get_code(e) != LV_EVENT_KEY) {
        return;
    }

    if(lv_event_get_key(e) == LV_KEY_ESC) {
        (void)PageManager_Pop();
    }
}

/**
 * @brief 周期刷新测试页面显示。
 */
static void test_page_timer_cb(lv_timer_t * timer)
{
    test_page_t * page = (test_page_t *)lv_timer_get_user_data(timer);

    test_page_update(page);
}

/**
 * @brief 创建固定宽度的状态文本。
 */
static lv_obj_t * test_page_create_label(lv_obj_t * parent, lv_coord_t y, lv_color_t color)
{
    lv_obj_t * label = lv_label_create(parent);

    if(label == NULL) {
        return NULL;
    }

    lv_obj_set_width(label, LV_HOR_RES - 24);
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_color(label, color, 0);
    lv_obj_set_style_text_font(label, LV_FONT_DEFAULT, 0);
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, 12, y);

    return label;
}

/**
 * @brief 从 HwAccess 缓存和 DebugOverlay 统计刷新页面文本。
 */
static void test_page_update(test_page_t * page)
{
    uint32_t lv_total = 0U;
    uint32_t lv_used = 0U;
    uint8_t battery_valid = 0U;
    uint8_t battery_percent = 0U;
    uint16_t battery_mv = 0U;
    HwAccess_Vector3i16_t accel;
    HwAccess_Vector3i16_t gyro;
    int16_t temp_x10;
    uint32_t step_count = 0U;

    if(page == NULL) {
        return;
    }

    DebugOverlay_GetLvMem(&lv_total, &lv_used);
    if(page->fps_label != NULL) {
        lv_label_set_text_fmt(page->fps_label,
                              "FPS:%lu\nLVGL:%lu/%lu B",
                              (unsigned long)DebugOverlay_GetFps(),
                              (unsigned long)lv_used,
                              (unsigned long)lv_total);
    }

    if((HwAccess.power.is_battery_valid != NULL) &&
       (HwAccess.power.get_battery_percent != NULL) &&
       (HwAccess.power.get_battery_voltage_mv != NULL) &&
       (HwAccess.power.is_battery_valid() != 0U)) {
        battery_valid = 1U;
        battery_percent = HwAccess.power.get_battery_percent();
        battery_mv = HwAccess.power.get_battery_voltage_mv();
    }

    if(page->power_label != NULL) {
        if(battery_valid == 0U) {
            lv_label_set_text(page->power_label, "BAT:--");
        } else {
            lv_label_set_text_fmt(page->power_label,
                                  "BAT:%u%% %umV",
                                  battery_percent,
                                  battery_mv);
        }
    }

    if(page->mpu_label == NULL) {
        return;
    }

    if(HwAccess.mpu6050.get_step_count != NULL) {
        step_count = HwAccess.mpu6050.get_step_count();
    }

    if((HwAccess.mpu6050.is_valid == NULL) ||
       (HwAccess.mpu6050.get_accel_mg == NULL) ||
       (HwAccess.mpu6050.get_gyro_x10_dps == NULL) ||
       (HwAccess.mpu6050.get_temperature_x10_c == NULL) ||
       (HwAccess.mpu6050.is_valid() == 0U) ||
       (HwAccess.mpu6050.get_accel_mg(&accel) != 0) ||
       (HwAccess.mpu6050.get_gyro_x10_dps(&gyro) != 0)) {
        lv_label_set_text_fmt(page->mpu_label,
                              "MPU6050: waiting\n"
                              "STEP:%lu",
                              (unsigned long)step_count);
        return;
    }

    temp_x10 = HwAccess.mpu6050.get_temperature_x10_c();
    lv_label_set_text_fmt(page->mpu_label,
                          "MPU6050 OK\n"
                          "STEP:%lu\n"
                          "ACC:%d,%d,%d mg\n"
                          "GYR:%d.%d,%d.%d,%d.%d dps\n"
                          "TMP:%d.%d C",
                          (unsigned long)step_count,
                          accel.x,
                          accel.y,
                          accel.z,
                          gyro.x / 10,
                          (gyro.x < 0) ? -(gyro.x % 10) : (gyro.x % 10),
                          gyro.y / 10,
                          (gyro.y < 0) ? -(gyro.y % 10) : (gyro.y % 10),
                          gyro.z / 10,
                          (gyro.z < 0) ? -(gyro.z % 10) : (gyro.z % 10),
                          temp_x10 / 10,
                          (temp_x10 < 0) ? -(temp_x10 % 10) : (temp_x10 % 10));
}

test_page_t * test_page_create(void)
{
    static test_page_t page_storage;
    test_page_t * page = &page_storage;
    lv_obj_t * title;

    memset(page, 0, sizeof(*page));

    page->root = lv_obj_create(NULL);
    if(page->root == NULL) {
        return NULL;
    }

    lv_obj_remove_style_all(page->root);
    lv_obj_set_size(page->root, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(page->root, lv_color_hex(0x0a0d12), 0);
    lv_obj_set_style_bg_opa(page->root, LV_OPA_COVER, 0);
    lv_obj_clear_flag(page->root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(page->root, test_page_key_cb, LV_EVENT_KEY, NULL);

    title = lv_label_create(page->root);
    if(title != NULL) {
        lv_label_set_text(title, "Test");
        lv_obj_set_style_text_color(title, lv_color_hex(0xffffff), 0);
        lv_obj_set_style_text_font(title, LV_FONT_DEFAULT, 0);
        lv_obj_align(title, LV_ALIGN_TOP_LEFT, 12, 10);
    }

    page->fps_label = test_page_create_label(page->root, 40, lv_color_hex(0x9be7ff));
    page->power_label = test_page_create_label(page->root, 92, lv_color_hex(0xb2f2bb));
    page->mpu_label = test_page_create_label(page->root, 124, lv_color_hex(0xffd8a8));

    test_page_update(page);

    page->timer = lv_timer_create(test_page_timer_cb, TEST_PAGE_REFRESH_MS, page);
    return page;
}

void test_page_destroy(test_page_t * page)
{
    if(page == NULL) {
        return;
    }

    if(page->timer != NULL) {
        lv_timer_del(page->timer);
        page->timer = NULL;
    }

    if(page->root != NULL) {
        lv_obj_del(page->root);
    }

    memset(page, 0, sizeof(*page));
}

lv_obj_t * test_page_root(test_page_t * page)
{
    return (page != NULL) ? page->root : NULL;
}

/**
 * @brief 创建并加载系统测试页面。
 */
static void TestPage_Create(void)
{
    lv_obj_t * root;

    if(s_test_page != NULL) {
        test_page_destroy(s_test_page);
        s_test_page = NULL;
    }

    s_test_page = test_page_create();
    if(s_test_page == NULL) {
        return;
    }

    root = test_page_root(s_test_page);
    if(root == NULL) {
        test_page_destroy(s_test_page);
        s_test_page = NULL;
        return;
    }

    lv_screen_load(root);
}

/**
 * @brief 释放系统测试页面。
 */
static void TestPage_Destroy(void)
{
    test_page_destroy(s_test_page);
    s_test_page = NULL;
}

const GUI_Page_t TestPage = {
    .create = TestPage_Create,
    .destroy = TestPage_Destroy,
};
