/**
 * @file calculator_page.c
 * @brief 计算器页面实现。
 *
 * 本页面使用 int64_t 定点数完成四则运算，避免在 STM32F411 目标平台上依赖
 * double、strtod 或 math 库。页面对象只在 LVGL UI 线程上下文中使用。
 */

#include "calculator_page.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define CALCULATOR_SCALE 1000000LL
#define CALCULATOR_FRACTION_DIGITS 6
#define CALCULATOR_MAX_ABS_VALUE 999999999LL

/**
 * @brief 计算器页面对象。
 *
 * root、history_display、display 都由本页面创建。history_display 和 display
 * 是 root 的间接子对象，root 删除后这些 LVGL 指针全部失效。
 */
struct calculator_page {
    lv_obj_t * root;               /**< 页面根 screen，归本页面对象所有。 */
    lv_obj_t * history_display;    /**< 历史表达式 label，root 删除时自动删除。 */
    lv_obj_t * display;            /**< 当前输入/结果 label，root 删除时自动删除。 */
    char history[64];              /**< 上方显示的历史表达式。 */
    char input[32];                /**< 下方显示的当前输入或结果。 */
    int64_t accumulator;           /**< 定点数累加器，实际值乘以 CALCULATOR_SCALE。 */
    char pending_op;               /**< 等待执行的运算符。 */
    bool entering;                 /**< true 表示用户正在输入当前数字。 */
    bool error;                    /**< true 表示当前计算进入错误状态。 */
};

static calculator_page_t * s_calculator_page;

static void calculator_reset(calculator_page_t * page)
{
    page->history[0] = '\0';
    lv_snprintf(page->input, sizeof(page->input), "0");
    page->accumulator = 0;
    page->pending_op = '\0';
    page->entering = true;
    page->error = false;
}

static int64_t calculator_abs64(int64_t value)
{
    return value < 0 ? -value : value;
}

static bool calculator_parse_input(const char * text, int64_t * out)
{
    bool negative = false;
    int64_t integer = 0;
    int64_t fraction = 0;
    int32_t fraction_digits = 0;
    bool after_dot = false;

    if(*text == '-') {
        negative = true;
        text++;
    }

    while(*text) {
        if(*text == '.') {
            if(after_dot) return false;
            after_dot = true;
            text++;
            continue;
        }

        if(*text < '0' || *text > '9') return false;

        if(after_dot) {
            if(fraction_digits < CALCULATOR_FRACTION_DIGITS) {
                fraction = fraction * 10 + (*text - '0');
                fraction_digits++;
            }
        }
        else {
            integer = integer * 10 + (*text - '0');
            if(integer > CALCULATOR_MAX_ABS_VALUE) return false;
        }

        text++;
    }

    while(fraction_digits < CALCULATOR_FRACTION_DIGITS) {
        fraction *= 10;
        fraction_digits++;
    }

    int64_t value = integer * CALCULATOR_SCALE + fraction;
    *out = negative ? -value : value;
    return true;
}

static void calculator_format_number(char * buf, size_t size, int64_t value)
{
    bool negative = value < 0;
    int64_t abs_value = calculator_abs64(value);
    int64_t integer = abs_value / CALCULATOR_SCALE;
    int64_t fraction = abs_value % CALCULATOR_SCALE;

    if(fraction == 0) {
        lv_snprintf(buf, size, negative ? "-%lld" : "%lld", integer);
        return;
    }

    char frac_buf[CALCULATOR_FRACTION_DIGITS + 1];
    lv_snprintf(frac_buf, sizeof(frac_buf), "%06lld", fraction);

    char * end = frac_buf + strlen(frac_buf) - 1;
    while(end > frac_buf && *end == '0') {
        *end = '\0';
        end--;
    }

    lv_snprintf(buf, size, negative ? "-%lld.%s" : "%lld.%s", integer, frac_buf);
}

static void calculator_refresh(calculator_page_t * page)
{
    lv_label_set_text(page->history_display, page->history);
    lv_label_set_text(page->display, page->error ? "Error" : page->input);
}

static void calculator_set_error(calculator_page_t * page)
{
    page->error = true;
    lv_snprintf(page->input, sizeof(page->input), "Error");
}

static bool calculator_add_checked(int64_t * total, int64_t value)
{
    if(value > 0 && *total > CALCULATOR_MAX_ABS_VALUE * CALCULATOR_SCALE - value) return false;
    *total += value;
    return true;
}

static bool calculator_mul_fixed(int64_t lhs, int64_t rhs, int64_t * out)
{
    bool negative = (lhs < 0) != (rhs < 0);
    int64_t a = calculator_abs64(lhs);
    int64_t b = calculator_abs64(rhs);

    int64_t ai = a / CALCULATOR_SCALE;
    int64_t af = a % CALCULATOR_SCALE;
    int64_t bi = b / CALCULATOR_SCALE;
    int64_t bf = b % CALCULATOR_SCALE;

    int64_t total = 0;

    if(ai != 0 && bi > CALCULATOR_MAX_ABS_VALUE / ai) return false;
    if(!calculator_add_checked(&total, ai * bi * CALCULATOR_SCALE)) return false;
    if(!calculator_add_checked(&total, ai * bf)) return false;
    if(!calculator_add_checked(&total, bi * af)) return false;
    if(!calculator_add_checked(&total, (af * bf) / CALCULATOR_SCALE)) return false;

    *out = negative ? -total : total;
    return true;
}

static bool calculator_div_fixed(int64_t lhs, int64_t rhs, int64_t * out)
{
    if(rhs == 0) return false;

    bool negative = (lhs < 0) != (rhs < 0);
    int64_t dividend = calculator_abs64(lhs);
    int64_t divisor = calculator_abs64(rhs);
    int64_t quotient = dividend / divisor;
    int64_t remainder = dividend % divisor;

    if(calculator_abs64(quotient) > CALCULATOR_MAX_ABS_VALUE) return false;

    int64_t scaled_integer = quotient * CALCULATOR_SCALE;
    int64_t scaled_fraction = 0;

    for(int32_t i = 0; i < CALCULATOR_FRACTION_DIGITS; i++) {
        remainder *= 10;
        scaled_fraction = scaled_fraction * 10 + remainder / divisor;
        remainder %= divisor;
    }

    *out = scaled_integer + scaled_fraction;
    if(negative) *out = -*out;
    return true;
}

static bool calculator_apply_pending(calculator_page_t * page, int64_t rhs)
{
    int64_t result = 0;

    switch(page->pending_op) {
        case '+':
            result = page->accumulator + rhs;
            break;
        case '-':
            result = page->accumulator - rhs;
            break;
        case '*':
            if(!calculator_mul_fixed(page->accumulator, rhs, &result)) {
                calculator_set_error(page);
                return false;
            }
            break;
        case '/':
            if(!calculator_div_fixed(page->accumulator, rhs, &result)) {
                calculator_set_error(page);
                return false;
            }
            break;
        default:
            page->accumulator = rhs;
            return true;
    }

    if(calculator_abs64(result) > CALCULATOR_MAX_ABS_VALUE * CALCULATOR_SCALE) {
        calculator_set_error(page);
        return false;
    }

    page->accumulator = result;
    return true;
}

static void calculator_input_digit(calculator_page_t * page, const char * digit)
{
    if(page->error) calculator_reset(page);

    if(!page->entering || strcmp(page->input, "0") == 0) {
        lv_snprintf(page->input, sizeof(page->input), "%s", digit);
        page->entering = true;
        return;
    }

    size_t len = strlen(page->input);
    if(len + 1 < sizeof(page->input)) {
        page->input[len] = digit[0];
        page->input[len + 1] = '\0';
    }
}

static void calculator_input_dot(calculator_page_t * page)
{
    if(page->error) calculator_reset(page);

    if(!page->entering) {
        lv_snprintf(page->input, sizeof(page->input), "0.");
        page->entering = true;
        return;
    }

    if(!strchr(page->input, '.')) {
        size_t len = strlen(page->input);
        if(len + 1 < sizeof(page->input)) {
            page->input[len] = '.';
            page->input[len + 1] = '\0';
        }
    }
}

static void calculator_backspace(calculator_page_t * page)
{
    if(page->error || !page->entering) {
        calculator_reset(page);
        return;
    }

    size_t len = strlen(page->input);
    if(len <= 1 || (len == 2 && page->input[0] == '-')) {
        lv_snprintf(page->input, sizeof(page->input), "0");
    }
    else {
        page->input[len - 1] = '\0';
    }
}

static void calculator_toggle_sign(calculator_page_t * page)
{
    if(page->error) calculator_reset(page);
    if(strcmp(page->input, "0") == 0) return;

    if(page->input[0] == '-') {
        memmove(page->input, page->input + 1, strlen(page->input));
    }
    else if(strlen(page->input) + 1 < sizeof(page->input)) {
        memmove(page->input + 1, page->input, strlen(page->input) + 1);
        page->input[0] = '-';
    }
}

static void calculator_percent(calculator_page_t * page)
{
    if(page->error) calculator_reset(page);

    int64_t value = 0;
    if(!calculator_parse_input(page->input, &value)) {
        calculator_set_error(page);
        return;
    }
    value /= 100;
    calculator_format_number(page->input, sizeof(page->input), value);
    page->entering = true;
}

static void calculator_set_operator(calculator_page_t * page, char op)
{
    if(page->error) calculator_reset(page);

    int64_t rhs = 0;
    if(!calculator_parse_input(page->input, &rhs)) {
        calculator_set_error(page);
        return;
    }
    if(page->entering || page->pending_op == '\0') {
        if(!calculator_apply_pending(page, rhs)) return;
        calculator_format_number(page->input, sizeof(page->input), page->accumulator);
    }

    page->pending_op = op;
    lv_snprintf(page->history, sizeof(page->history), "%s %c", page->input, op);
    page->entering = false;
}

static void calculator_equal(calculator_page_t * page)
{
    if(page->error) {
        calculator_reset(page);
        return;
    }

    int64_t rhs = 0;
    if(!calculator_parse_input(page->input, &rhs)) {
        calculator_set_error(page);
        return;
    }
    char lhs_text[32];
    char rhs_text[32];
    calculator_format_number(lhs_text, sizeof(lhs_text), page->pending_op ? page->accumulator : rhs);
    calculator_format_number(rhs_text, sizeof(rhs_text), rhs);
    char op = page->pending_op;

    if(!calculator_apply_pending(page, rhs)) return;

    calculator_format_number(page->input, sizeof(page->input), page->accumulator);
    if(op) lv_snprintf(page->history, sizeof(page->history), "%s %c %s =", lhs_text, op, rhs_text);
    else lv_snprintf(page->history, sizeof(page->history), "%s =", rhs_text);
    page->pending_op = '\0';
    page->entering = false;
}

static void calculator_page_key_cb(lv_event_t * e)
{
    if(lv_event_get_code(e) != LV_EVENT_KEY) return;
    if(lv_event_get_key(e) == LV_KEY_ESC) (void)PageManager_Pop();
}

static void calculator_button_cb(lv_event_t * e)
{
    if(lv_event_get_code(e) != LV_EVENT_CLICKED) return;

    calculator_page_t * page = (calculator_page_t *)lv_event_get_user_data(e);
    lv_obj_t * label = lv_obj_get_child(lv_event_get_target(e), 0);
    const char * key = lv_label_get_text(label);

    if(strcmp(key, "C") == 0) calculator_reset(page);
    else if(strcmp(key, "<") == 0) calculator_backspace(page);
    else if(strcmp(key, "+/-") == 0) calculator_toggle_sign(page);
    else if(strcmp(key, "%") == 0) calculator_percent(page);
    else if(strcmp(key, ".") == 0) calculator_input_dot(page);
    else if(strcmp(key, "=") == 0) calculator_equal(page);
    else if(strcmp(key, "+") == 0) calculator_set_operator(page, '+');
    else if(strcmp(key, "-") == 0) calculator_set_operator(page, '-');
    else if(strcmp(key, "x") == 0) calculator_set_operator(page, '*');
    else if(strcmp(key, "/") == 0) calculator_set_operator(page, '/');
    else calculator_input_digit(page, key);

    calculator_refresh(page);
}

static lv_obj_t * calculator_button_create(lv_obj_t * parent, calculator_page_t * page, const char * text,
                                           lv_color_t bg, lv_color_t fg, int32_t width, int32_t height)
{
    lv_obj_t * btn = lv_btn_create(parent);
    lv_obj_set_size(btn, width, height);
    lv_obj_set_style_radius(btn, height / 3 > 14 ? 14 : height / 3, 0);
    lv_obj_set_style_bg_color(btn, bg, 0);
    lv_obj_set_style_bg_color(btn, lv_color_mix(bg, lv_color_white(), 40), LV_STATE_PRESSED);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_add_event_cb(btn, calculator_button_cb, LV_EVENT_CLICKED, page);
    lv_obj_add_event_cb(btn, calculator_page_key_cb, LV_EVENT_KEY, NULL);

    lv_obj_t * label = lv_label_create(btn);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, fg, 0);
    lv_obj_set_style_text_font(label, LV_FONT_DEFAULT, 0);
    lv_obj_center(label);

    return btn;
}

calculator_page_t * calculator_page_create(void)
{
    static const char * keys[] = {
        "C", "+/-", "%", "/",
        "7", "8", "9", "x",
        "4", "5", "6", "-",
        "1", "2", "3", "+",
        "<", "0", ".", "=",
    };

    static calculator_page_t page_storage;
    calculator_page_t * page = &page_storage;
    memset(page, 0, sizeof(*page));
    calculator_reset(page);

    int32_t page_w = LV_HOR_RES;
    int32_t page_h = LV_VER_RES;
    int32_t margin = page_h <= 360 ? 10 : 16;
    int32_t display_y = margin;
    int32_t display_h = page_h <= 360 ? 84 : 104;
    int32_t grid_y = display_y + display_h + (page_h <= 360 ? 10 : 12);
    int32_t grid_h = page_h - grid_y - margin;
    int32_t row_gap = page_h <= 360 ? 5 : 8;
    int32_t col_gap = page_w <= 260 ? 6 : 8;
    int32_t button_h = (grid_h - row_gap * 4) / 5;
    if(button_h < 28) button_h = 28;
    int32_t button_w = (page_w - margin * 2 - col_gap * 3) / 4;

    page->root = lv_obj_create(NULL);
    lv_obj_set_size(page->root, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(page->root, lv_color_hex(0x0b0f12), 0);
    lv_obj_set_style_border_width(page->root, 0, 0);
    lv_obj_set_style_pad_all(page->root, 0, 0);
    lv_obj_set_scrollbar_mode(page->root, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_event_cb(page->root, calculator_page_key_cb, LV_EVENT_KEY, NULL);

    lv_obj_t * display_panel = lv_obj_create(page->root);
    lv_obj_set_size(display_panel, page_w - margin * 2, display_h);
    lv_obj_set_pos(display_panel, margin, display_y);
    lv_obj_set_style_bg_color(display_panel, lv_color_hex(0x12181f), 0);
    lv_obj_set_style_bg_opa(display_panel, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(display_panel, 1, 0);
    lv_obj_set_style_border_color(display_panel, lv_color_hex(0x26313b), 0);
    lv_obj_set_style_radius(display_panel, 12, 0);
    lv_obj_set_style_pad_all(display_panel, page_h <= 360 ? 7 : 12, 0);
    lv_obj_clear_flag(display_panel, LV_OBJ_FLAG_SCROLLABLE);

    page->history_display = lv_label_create(display_panel);
    lv_label_set_text(page->history_display, page->history);
    lv_label_set_long_mode(page->history_display, LV_LABEL_LONG_DOT);
    lv_obj_set_width(page->history_display, lv_pct(100));
    lv_obj_set_style_text_align(page->history_display, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_style_text_color(page->history_display, lv_color_hex(0x868e96), 0);
    lv_obj_set_style_text_font(page->history_display, LV_FONT_DEFAULT, 0);
    lv_obj_align(page->history_display, LV_ALIGN_TOP_RIGHT, 0, 0);

    page->display = lv_label_create(display_panel);
    lv_label_set_text(page->display, page->input);
    lv_label_set_long_mode(page->display, LV_LABEL_LONG_DOT);
    lv_obj_set_width(page->display, lv_pct(100));
    lv_obj_set_style_text_align(page->display, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_style_text_color(page->display, lv_color_white(), 0);
    lv_obj_set_style_text_font(page->display, LV_FONT_DEFAULT, 0);
    lv_obj_align(page->display, LV_ALIGN_BOTTOM_RIGHT, 0, 0);

    lv_obj_t * grid = lv_obj_create(page->root);
    lv_obj_set_size(grid, page_w - margin * 2, grid_h);
    lv_obj_set_pos(grid, margin, grid_y);
    lv_obj_set_style_bg_opa(grid, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(grid, 0, 0);
    lv_obj_set_style_pad_all(grid, 0, 0);
    lv_obj_clear_flag(grid, LV_OBJ_FLAG_SCROLLABLE);

    for(size_t i = 0; i < sizeof(keys) / sizeof(keys[0]); i++) {
        bool op = strcmp(keys[i], "/") == 0 || strcmp(keys[i], "x") == 0 ||
                  strcmp(keys[i], "-") == 0 || strcmp(keys[i], "+") == 0 ||
                  strcmp(keys[i], "=") == 0;
        bool util = i < 3 || strcmp(keys[i], "<") == 0;
        lv_color_t bg = op ? lv_color_hex(0xffa94d) : (util ? lv_color_hex(0x2b3440) : lv_color_hex(0x192027));
        lv_obj_t * btn = calculator_button_create(grid, page, keys[i], bg, lv_color_white(), button_w, button_h);
        lv_obj_set_pos(btn, (int32_t)(i % 4) * (button_w + col_gap), (int32_t)(i / 4) * (button_h + row_gap));
    }

    return page;
}

void calculator_page_destroy(calculator_page_t * page)
{
    if(!page) return;
    if(page->root) lv_obj_del(page->root);
    memset(page, 0, sizeof(*page));
}

lv_obj_t * calculator_page_root(calculator_page_t * page)
{
    return page ? page->root : NULL;
}

/**
 * @brief 创建并加载计算器页面。
 */
static void CalculatorPage_Create(void)
{
    lv_obj_t * root;

    if(s_calculator_page != NULL) {
        calculator_page_destroy(s_calculator_page);
        s_calculator_page = NULL;
    }

    s_calculator_page = calculator_page_create();
    if(s_calculator_page == NULL) {
        return;
    }

    root = calculator_page_root(s_calculator_page);
    if(root == NULL) {
        calculator_page_destroy(s_calculator_page);
        s_calculator_page = NULL;
        return;
    }

    lv_screen_load(root);
}

/**
 * @brief 释放计算器页面。
 */
static void CalculatorPage_Destroy(void)
{
    calculator_page_destroy(s_calculator_page);
    s_calculator_page = NULL;
}

const GUI_Page_t CalculatorPage = {
    .create = CalculatorPage_Create,
    .destroy = CalculatorPage_Destroy,
};
