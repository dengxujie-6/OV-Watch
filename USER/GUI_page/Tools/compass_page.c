/**
 * @file compass_page.c
 * @brief 指南针页面实现。
 *
 * 页面只读取 HwAccess 缓存，不直接访问 BSP/I2C。LSM303DLHC 的真实采样由 Sensor_Task 完成。
 */

#include "compass_page.h"

#include <math.h>
#include <stdint.h>
#include <string.h>

#include "hwaccess.h"

#define COMPASS_REFRESH_MS         250U
#define COMPASS_NEEDLE_X           109
#define COMPASS_NEEDLE_Y           55
#define COMPASS_NEEDLE_PIVOT_X     11
#define COMPASS_NEEDLE_PIVOT_Y     85
#define COMPASS_PI                3.14159265f

#define COMPASS_DIAL_WIDTH         240
#define COMPASS_DIAL_HEIGHT        280
#define COMPASS_DIAL_CENTER_X      120
#define COMPASS_DIAL_CENTER_Y      140
#define COMPASS_DIAL_TICK_RADIUS   108
#define COMPASS_DIAL_SHORT_LEN     5
#define COMPASS_DIAL_LONG_LEN      10
#define COMPASS_DIAL_MAIN_LEN      15
#define COMPASS_DIAL_TEXT_RADIUS   91
#define COMPASS_DIAL_DIR_RADIUS    70
#define COMPASS_DIAL_SUBDIR_RADIUS 76
#define COMPASS_DIAL_CANVAS_CF     LV_COLOR_FORMAT_I1
#define COMPASS_INFO_CARD_WIDTH    100
#define COMPASS_INFO_CARD_HEIGHT   30
#define COMPASS_INFO_CARD_Y        242
#define COMPASS_INFO_DIR_X         12
#define COMPASS_INFO_ALT_X         128

enum {
    COMPASS_DIAL_COLOR_BLACK = 0,
    COMPASS_DIAL_COLOR_WHITE,
};

/**
 * @brief 指南针页面对象。
 */
struct compass_page {
    lv_obj_t * root;             /**< 页面根 screen，归本页面对象所有。 */
    lv_obj_t * needle;           /**< 指针图片对象，root 删除时自动删除。 */
    lv_obj_t * heading_label;    /**< 底部方向数值标签，随 root 自动删除。 */
    lv_obj_t * altitude_label;   /**< 底部高度数值标签，随 root 自动删除。 */
    lv_timer_t * refresh_timer;  /**< 页面刷新定时器，由本页面删除。 */
};

static compass_page_t * s_compass_page;
static lv_obj_t * s_compass_dial_root;
static const lv_point_precise_t s_compass_marker_points[] = {
    { 7, 0 },
    { 0, 13 },
    { 14, 13 },
    { 7, 0 },
};
static LV_ATTRIBUTE_MEM_ALIGN uint8_t s_compass_dial_buf[LV_DRAW_BUF_SIZE(COMPASS_DIAL_WIDTH,
                                                                          COMPASS_DIAL_HEIGHT,
                                                                          COMPASS_DIAL_CANVAS_CF)];

static void compass_dial_point(int32_t angle_deg, int32_t radius, lv_point_precise_t * point);
static int32_t compass_dial_round(float value);
static lv_color32_t compass_dial_color32(uint8_t red, uint8_t green, uint8_t blue);
static void compass_dial_set_px(lv_obj_t * canvas, int32_t x, int32_t y, uint8_t color_index);
static void compass_dial_draw_line(lv_obj_t * canvas, lv_point_precise_t p1, lv_point_precise_t p2,
                                   uint8_t color_index, int32_t width);
static void compass_dial_draw_circle(lv_obj_t * canvas, int32_t radius, int32_t width, uint8_t color_index);
static void compass_dial_add_marker(void);
static void compass_dial_draw_canvas(lv_obj_t * canvas);
static void compass_dial_add_label(const char * text, int32_t angle_deg, int32_t radius, int32_t width,
                                   int32_t height, lv_color_t color);
static void compass_page_key_cb(lv_event_t * e);
static void compass_page_timer_cb(lv_timer_t * timer);
static void compass_page_update(compass_page_t * page);
static lv_obj_t * compass_page_create_info_card(lv_obj_t * parent, int32_t x, const char * title,
                                                const char * value_text);
static uint16_t compass_heading_from_mag(const HwAccess_Vector3i16_t * mag);

LV_IMAGE_DECLARE(ui_img_compass_needle_png);

/**
 * @brief 将罗盘角度转换为画布坐标。
 *
 * 角度定义为 0 度在屏幕正上方，顺时针递增；point 允许为 NULL。
 */
static void compass_dial_point(int32_t angle_deg, int32_t radius, lv_point_precise_t * point)
{
    float rad;

    if(point == NULL) {
        return;
    }

    rad = (float)angle_deg * (COMPASS_PI / 180.0f);
    point->x = (lv_value_precise_t)(COMPASS_DIAL_CENTER_X + compass_dial_round(sinf(rad) * (float)radius));
    point->y = (lv_value_precise_t)(COMPASS_DIAL_CENTER_Y - compass_dial_round(cosf(rad) * (float)radius));
}

/**
 * @brief 四舍五入浮点坐标，兼容正负方向。
 */
static int32_t compass_dial_round(float value)
{
    return (value >= 0.0f) ? (int32_t)(value + 0.5f) : (int32_t)(value - 0.5f);
}

/**
 * @brief 生成 indexed canvas 调色板颜色。
 */
static lv_color32_t compass_dial_color32(uint8_t red, uint8_t green, uint8_t blue)
{
    lv_color32_t color;

    color.red = red;
    color.green = green;
    color.blue = blue;
    color.alpha = 0xffU;

    return color;
}

/**
 * @brief 在 I4 canvas 上写入一个像素索引。
 */
static void compass_dial_set_px(lv_obj_t * canvas, int32_t x, int32_t y, uint8_t color_index)
{
    lv_draw_buf_t * draw_buf;
    uint8_t * data;
    uint8_t bpp;
    uint8_t shift;
    uint8_t mask;

    if((canvas == NULL) ||
       (x < 0) || (x >= COMPASS_DIAL_WIDTH) ||
       (y < 0) || (y >= COMPASS_DIAL_HEIGHT)) {
        return;
    }

    draw_buf = lv_canvas_get_draw_buf(canvas);
    if(draw_buf == NULL) {
        return;
    }

    data = (uint8_t *)lv_draw_buf_goto_xy(draw_buf, (uint32_t)x, (uint32_t)y);
    if(data == NULL) {
        return;
    }

    bpp = lv_color_format_get_bpp((lv_color_format_t)draw_buf->header.cf);
    if((bpp == 0U) || (bpp > 4U)) {
        return;
    }

    shift = (uint8_t)(8U - bpp - (bpp * ((uint8_t)x & ((8U / bpp) - 1U))));
    mask = (uint8_t)(((1U << bpp) - 1U) << shift);
    *data = (uint8_t)((*data & ~mask) | ((color_index & ((1U << bpp) - 1U)) << shift));
}

/**
 * @brief 在 canvas 上绘制带简单粗细的线段。
 */
static void compass_dial_draw_line(lv_obj_t * canvas, lv_point_precise_t p1, lv_point_precise_t p2,
                                   uint8_t color_index, int32_t width)
{
    int32_t x0 = (int32_t)p1.x;
    int32_t y0 = (int32_t)p1.y;
    int32_t x1 = (int32_t)p2.x;
    int32_t y1 = (int32_t)p2.y;
    int32_t dx = LV_ABS(x1 - x0);
    int32_t sx = (x0 < x1) ? 1 : -1;
    int32_t dy = -LV_ABS(y1 - y0);
    int32_t sy = (y0 < y1) ? 1 : -1;
    int32_t err = dx + dy;
    int32_t half = width / 2;
    int32_t e2;
    int32_t ox;
    int32_t oy;

    while(1) {
        for(oy = -half; oy <= half; oy++) {
            for(ox = -half; ox <= half; ox++) {
                compass_dial_set_px(canvas, x0 + ox, y0 + oy, color_index);
            }
        }

        if((x0 == x1) && (y0 == y1)) {
            break;
        }

        e2 = 2 * err;
        if(e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if(e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

/**
 * @brief 用短线段近似绘制圆环，避免创建额外 arc 对象。
 */
static void compass_dial_draw_circle(lv_obj_t * canvas, int32_t radius, int32_t width, uint8_t color_index)
{
    lv_point_precise_t p1;
    lv_point_precise_t p2;
    int32_t angle;

    compass_dial_point(0, radius, &p1);
    for(angle = 4; angle <= 360; angle += 4) {
        compass_dial_point(angle, radius, &p2);
        compass_dial_draw_line(canvas, p1, p2, color_index, width);
        p1 = p2;
    }
}

/**
 * @brief 绘制罗盘 canvas：刻度、圆环和顶部标记。
 */
static void compass_dial_draw_canvas(lv_obj_t * canvas)
{
    lv_draw_buf_t * draw_buf;
    uint8_t * canvas_data;
    lv_point_precise_t outer;
    lv_point_precise_t inner;
    int32_t angle;
    int32_t tick_len;
    int32_t tick_width;

    draw_buf = lv_canvas_get_draw_buf(canvas);
    if(draw_buf == NULL) {
        return;
    }

    canvas_data = (uint8_t *)lv_draw_buf_goto_xy(draw_buf, 0U, 0U);
    if(canvas_data == NULL) {
        return;
    }

    // 只清像素区，保留 I1 调色板；黑色背景对应索引 0。
    memset(canvas_data, 0, draw_buf->header.stride * draw_buf->header.h);

    // 外圈淡圆和中心圆环都在 canvas 内绘制，减少 LVGL 对象数量。
    compass_dial_draw_circle(canvas, COMPASS_DIAL_TICK_RADIUS, 1, COMPASS_DIAL_COLOR_WHITE);
    compass_dial_draw_circle(canvas, 10, 2, COMPASS_DIAL_COLOR_WHITE);

    for(angle = 0; angle < 360; angle += 5) {
        if((angle % 90) == 0) {
            tick_len = COMPASS_DIAL_MAIN_LEN;
            tick_width = 3;
        }
        else if((angle % 30) == 0) {
            tick_len = COMPASS_DIAL_LONG_LEN;
            tick_width = 2;
        }
        else {
            tick_len = COMPASS_DIAL_SHORT_LEN;
            tick_width = 1;
        }

        compass_dial_point(angle, COMPASS_DIAL_TICK_RADIUS, &outer);
        compass_dial_point(angle, COMPASS_DIAL_TICK_RADIUS - tick_len, &inner);
        compass_dial_draw_line(canvas, inner, outer, COMPASS_DIAL_COLOR_WHITE, tick_width);
    }

    lv_obj_invalidate(canvas);
}

/**
 * @brief 添加顶部 0 度红色小三角。
 */
static void compass_dial_add_marker(void)
{
    lv_obj_t * marker;

    if(s_compass_dial_root == NULL) {
        return;
    }

    marker = lv_line_create(s_compass_dial_root);
    if(marker == NULL) {
        return;
    }

    lv_line_set_points(marker, s_compass_marker_points,
                       sizeof(s_compass_marker_points) / sizeof(s_compass_marker_points[0]));
    lv_obj_set_pos(marker, COMPASS_DIAL_CENTER_X - 7, 25);
    lv_obj_set_style_line_color(marker, lv_color_hex(0xff3030), 0);
    lv_obj_set_style_line_width(marker, 2, 0);
    lv_obj_set_style_line_rounded(marker, true, 0);
    lv_obj_clear_flag(marker, LV_OBJ_FLAG_CLICKABLE);
}

/**
 * @brief 在罗盘根容器上添加一个居中的文字标签。
 */
static void compass_dial_add_label(const char * text, int32_t angle_deg, int32_t radius, int32_t width,
                                   int32_t height, lv_color_t color)
{
    lv_obj_t * label;
    lv_point_precise_t pos;

    if((s_compass_dial_root == NULL) || (text == NULL)) {
        return;
    }

    compass_dial_point(angle_deg, radius, &pos);

    label = lv_label_create(s_compass_dial_root);
    if(label == NULL) {
        return;
    }

    lv_obj_remove_style_all(label);
    lv_obj_set_size(label, width, height);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, color, 0);
    lv_obj_set_style_text_opa(label, LV_OPA_COVER, 0);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_bg_opa(label, LV_OPA_TRANSP, 0);
    lv_obj_set_pos(label, (int32_t)pos.x - (width / 2), (int32_t)pos.y - (height / 2));
    lv_obj_clear_flag(label, LV_OBJ_FLAG_CLICKABLE);
}

void CompassDial_Create(lv_obj_t * parent)
{
    lv_obj_t * canvas;
    static const char * const angle_texts[12] = {
        "0", "30", "60", "90", "120", "150", "180", "210", "240", "270", "300", "330"
    };
    static const char * const main_dirs[4] = { "N", "E", "S", "W" };
    static const char * const sub_dirs[4] = { "NE", "SE", "SW", "NW" };
    int32_t i;

    if(parent == NULL) {
        return;
    }

    CompassDial_Destroy();

    s_compass_dial_root = lv_obj_create(parent);
    if(s_compass_dial_root == NULL) {
        return;
    }

    lv_obj_remove_style_all(s_compass_dial_root);
    lv_obj_set_size(s_compass_dial_root, COMPASS_DIAL_WIDTH, COMPASS_DIAL_HEIGHT);
    lv_obj_set_pos(s_compass_dial_root, 0, 0);
    lv_obj_set_style_bg_color(s_compass_dial_root, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(s_compass_dial_root, LV_OPA_COVER, 0);
    lv_obj_clear_flag(s_compass_dial_root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(s_compass_dial_root, LV_OBJ_FLAG_CLICKABLE);

    canvas = lv_canvas_create(s_compass_dial_root);
    if(canvas == NULL) {
        CompassDial_Destroy();
        return;
    }

    lv_canvas_set_buffer(canvas, s_compass_dial_buf, COMPASS_DIAL_WIDTH, COMPASS_DIAL_HEIGHT, COMPASS_DIAL_CANVAS_CF);
    lv_canvas_set_palette(canvas, COMPASS_DIAL_COLOR_BLACK, compass_dial_color32(0x00U, 0x00U, 0x00U));
    lv_canvas_set_palette(canvas, COMPASS_DIAL_COLOR_WHITE, compass_dial_color32(0xf4U, 0xf6U, 0xfaU));
    lv_obj_set_pos(canvas, 0, 0);
    lv_obj_clear_flag(canvas, LV_OBJ_FLAG_CLICKABLE);
    compass_dial_draw_canvas(canvas);
    compass_dial_add_marker();

    for(i = 0; i < 12; i++) {
        compass_dial_add_label(angle_texts[i], i * 30, COMPASS_DIAL_TEXT_RADIUS, 36, 16, lv_color_hex(0xb8bec8));
    }

    for(i = 0; i < 4; i++) {
        compass_dial_add_label(main_dirs[i], i * 90, COMPASS_DIAL_DIR_RADIUS, 32, 20,
                               (i == 0) ? lv_color_hex(0xff3030) : lv_color_hex(0xf4f6fa));
    }

    for(i = 0; i < 4; i++) {
        compass_dial_add_label(sub_dirs[i], 45 + (i * 90), COMPASS_DIAL_SUBDIR_RADIUS, 34, 18,
                               lv_color_hex(0x7f8792));
    }
}

void CompassDial_Destroy(void)
{
    if(s_compass_dial_root != NULL) {
        lv_obj_del(s_compass_dial_root);
        s_compass_dial_root = NULL;
    }
}

/**
 * @brief 处理返回按键。
 */
static void compass_page_key_cb(lv_event_t * e)
{
    if(lv_event_get_code(e) != LV_EVENT_KEY) {
        return;
    }

    if(lv_event_get_key(e) == LV_KEY_ESC) {
        (void)PageManager_Pop();
    }
}

/**
 * @brief 定时刷新指南针显示。
 */
static void compass_page_timer_cb(lv_timer_t * timer)
{
    compass_page_t * page = (compass_page_t *)lv_timer_get_user_data(timer);

    compass_page_update(page);
}

/**
 * @brief 创建底部半透明信息块，并返回其中的数值标签。
 *
 * @param parent 父对象，不能为 NULL。
 * @param x 信息块左上角 X 坐标。
 * @param title 左侧短标题文本，不能为 NULL。
 * @param value_text 初始数值文本，不能为 NULL。
 * @return 成功返回数值标签对象，失败返回 NULL；对象生命周期由 parent 管理。
 */
static lv_obj_t * compass_page_create_info_card(lv_obj_t * parent, int32_t x, const char * title,
                                                const char * value_text)
{
    lv_obj_t * card;
    lv_obj_t * title_label;
    lv_obj_t * value_label;

    if((parent == NULL) || (title == NULL) || (value_text == NULL)) {
        return NULL;
    }

    card = lv_obj_create(parent);
    if(card == NULL) {
        return NULL;
    }

    lv_obj_remove_style_all(card);
    lv_obj_set_size(card, COMPASS_INFO_CARD_WIDTH, COMPASS_INFO_CARD_HEIGHT);
    lv_obj_set_pos(card, x, COMPASS_INFO_CARD_Y);
    lv_obj_set_style_radius(card, 5, 0);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x808080), 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_50, 0);
    lv_obj_set_style_border_width(card, 0, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    title_label = lv_label_create(card);
    if(title_label != NULL) {
        lv_obj_remove_style_all(title_label);
        lv_obj_set_size(title_label, 32, COMPASS_INFO_CARD_HEIGHT);
        lv_obj_set_pos(title_label, 7, 0);
        lv_label_set_text(title_label, title);
        lv_obj_set_style_text_color(title_label, lv_color_hex(0xb8bec8), 0);
        lv_obj_set_style_text_align(title_label, LV_TEXT_ALIGN_LEFT, 0);
        lv_obj_align(title_label, LV_ALIGN_LEFT_MID, 7, 0);
    }

    value_label = lv_label_create(card);
    if(value_label == NULL) {
        return NULL;
    }

    lv_obj_remove_style_all(value_label);
    lv_obj_set_size(value_label, 56, COMPASS_INFO_CARD_HEIGHT);
    lv_label_set_text(value_label, value_text);
    lv_obj_set_style_text_color(value_label, lv_color_hex(0xf4f6fa), 0);
    lv_obj_set_style_text_align(value_label, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_align(value_label, LV_ALIGN_LEFT_MID, 38, 0);
    lv_obj_clear_flag(value_label, LV_OBJ_FLAG_CLICKABLE);

    return value_label;
}

/**
 * @brief 从 LSM303DLHC 缓存刷新航向角、指针和调试数值。
 */
static void compass_page_update(compass_page_t * page)
{
    HwAccess_Vector3i16_t mag;
    uint16_t heading;
    char heading_text[8];

    if((page == NULL) || (page->root == NULL)) {
        return;
    }

    if((HwAccess.lsm303dlhc.is_valid == NULL) ||
       (HwAccess.lsm303dlhc.get_mag_mgauss == NULL) ||
       (HwAccess.lsm303dlhc.is_valid() == 0U) ||
       (HwAccess.lsm303dlhc.get_mag_mgauss(&mag) != 0)) {
        return;
    }

    heading = compass_heading_from_mag(&mag);

    // 指针图片默认朝上；LVGL 图片旋转单位为 0.1 度，顺时针正方向。
    if(page->needle != NULL) {
        lv_image_set_rotation(page->needle, (int32_t)heading * 10);
    }

    if(page->heading_label != NULL) {
        lv_snprintf(heading_text, sizeof(heading_text), ":%03u", (unsigned int)heading);
        lv_label_set_text(page->heading_label, heading_text);
    }
}

/**
 * @brief 根据水平面磁场分量计算 0~359 度航向。
 */
static uint16_t compass_heading_from_mag(const HwAccess_Vector3i16_t * mag)
{
    float angle;
    int32_t heading;

    if(mag == NULL) {
        return 0U;
    }

    angle = atan2f((float)mag->y, (float)mag->x) * (180.0f / COMPASS_PI);
    if(angle < 0.0f) {
        angle += 360.0f;
    }

    heading = (int32_t)(angle + 0.5f);
    if(heading >= 360) {
        heading -= 360;
    }

    return (uint16_t)heading;
}

compass_page_t * compass_page_create(void)
{
    static compass_page_t page_storage;
    compass_page_t * page = &page_storage;

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
    lv_obj_add_event_cb(page->root, compass_page_key_cb, LV_EVENT_KEY, NULL);

    CompassDial_Create(page->root);

    page->needle = lv_image_create(page->root);
    lv_image_set_src(page->needle, &ui_img_compass_needle_png);
    lv_image_set_pivot(page->needle, COMPASS_NEEDLE_PIVOT_X, COMPASS_NEEDLE_PIVOT_Y);
    lv_obj_set_pos(page->needle, COMPASS_NEEDLE_X, COMPASS_NEEDLE_Y);
    lv_obj_clear_flag(page->needle, LV_OBJ_FLAG_CLICKABLE);

    page->heading_label = compass_page_create_info_card(page->root, COMPASS_INFO_DIR_X, "DIR", ":---");
    page->altitude_label = compass_page_create_info_card(page->root, COMPASS_INFO_ALT_X, "ALT", ":--m");

    page->refresh_timer = lv_timer_create(compass_page_timer_cb, COMPASS_REFRESH_MS, page);
    compass_page_update(page);

    return page;
}

void compass_page_destroy(compass_page_t * page)
{
    if(page == NULL) {
        return;
    }

    if(page->refresh_timer != NULL) {
        lv_timer_del(page->refresh_timer);
        page->refresh_timer = NULL;
    }

    if(page->root != NULL) {
        CompassDial_Destroy();
        lv_obj_del(page->root);
    }

    memset(page, 0, sizeof(*page));
}

lv_obj_t * compass_page_root(compass_page_t * page)
{
    return (page != NULL) ? page->root : NULL;
}

/**
 * @brief 创建并加载指南针页面。
 */
static void CompassPage_Create(void)
{
    lv_obj_t * root;

    if(s_compass_page != NULL) {
        compass_page_destroy(s_compass_page);
        s_compass_page = NULL;
    }

    s_compass_page = compass_page_create();
    if(s_compass_page == NULL) {
        return;
    }

    root = compass_page_root(s_compass_page);
    if(root == NULL) {
        compass_page_destroy(s_compass_page);
        s_compass_page = NULL;
        return;
    }

    lv_screen_load(root);
}

/**
 * @brief 释放指南针页面。
 */
static void CompassPage_Destroy(void)
{
    compass_page_destroy(s_compass_page);
    s_compass_page = NULL;
}

const GUI_Page_t CompassPage = {
    .create = CompassPage_Create,
    .destroy = CompassPage_Destroy,
};
