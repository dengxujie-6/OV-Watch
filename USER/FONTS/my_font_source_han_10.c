/*******************************************************************************
 * Size: 10 px
 * Bpp: 2
 * Opts: --bpp 2 --size 10 --no-compress --stride 1 --align 1 --font NotoSansSC-Regular.ttf --symbols ℃％/， --range 32-127 --format lvgl -o my_font_source_han_10.c
 ******************************************************************************/

#ifdef __has_include
    #if __has_include("lvgl.h")
        #ifndef LV_LVGL_H_INCLUDE_SIMPLE
            #define LV_LVGL_H_INCLUDE_SIMPLE
        #endif
    #endif
#endif

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
    #include "lvgl.h"
#else
    #include "lvgl/lvgl.h"
#endif



#ifndef MY_FONT_SOURCE_HAN_10
#define MY_FONT_SOURCE_HAN_10 1
#endif

#if MY_FONT_SOURCE_HAN_10

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0020 " " */

    /* U+0021 "!" */
    0x30, 0xc3, 0xc, 0x20, 0x41, 0xc,

    /* U+0022 "\"" */
    0x33, 0x33, 0x22, 0x0,

    /* U+0023 "#" */
    0x4, 0x81, 0x44, 0x7b, 0xc2, 0x10, 0x22, 0x7,
    0xb8, 0x22, 0x2, 0x20,

    /* U+0024 "$" */
    0x0, 0x2, 0x1, 0xf4, 0xc0, 0x30, 0x7, 0x40,
    0x34, 0x3, 0x0, 0xcb, 0xc0, 0x80,

    /* U+0025 "%" */
    0x28, 0x8, 0x20, 0x84, 0x8, 0x22, 0x1, 0x49,
    0x28, 0x28, 0x94, 0x80, 0x44, 0x20, 0x21, 0x48,
    0x10, 0x29,

    /* U+0026 "&" */
    0xb, 0x0, 0x85, 0x2, 0x60, 0x7, 0x0, 0x39,
    0x26, 0x4d, 0xc9, 0xe, 0xb, 0xd9,

    /* U+0027 "'" */
    0x33, 0x20,

    /* U+0028 "(" */
    0x4, 0x52, 0xc, 0x20, 0x82, 0xc, 0x20, 0x50,
    0x80,

    /* U+0029 ")" */
    0x40, 0x82, 0x8, 0x14, 0x51, 0x45, 0x20, 0x85,
    0x0,

    /* U+002A "*" */
    0x8, 0xb, 0x82, 0xc0, 0x40,

    /* U+002B "+" */
    0x4, 0x0, 0x80, 0x8, 0x6, 0xf8, 0x8, 0x0,
    0x80,

    /* U+002C "," */
    0x10, 0xc2, 0x14, 0x0,

    /* U+002D "-" */
    0x7c,

    /* U+002E "." */
    0x10, 0xc0,

    /* U+002F "/" */
    0x2, 0x5, 0x8, 0x8, 0x4, 0x10, 0x20, 0x20,
    0x50, 0x80, 0x40,

    /* U+0030 "0" */
    0x1f, 0x3, 0x18, 0x50, 0xc5, 0xc, 0x50, 0xc6,
    0xc, 0x31, 0x81, 0xf0,

    /* U+0031 "1" */
    0x1d, 0x2, 0x40, 0x50, 0x14, 0x5, 0x1, 0x40,
    0x50, 0xfe,

    /* U+0032 "2" */
    0x2f, 0x4, 0x18, 0x0, 0x80, 0x14, 0x3, 0x0,
    0x90, 0x24, 0x7, 0xfc,

    /* U+0033 "3" */
    0x2f, 0x0, 0x60, 0x18, 0x3c, 0x1, 0x80, 0x34,
    0xc, 0xfc,

    /* U+0034 "4" */
    0x3, 0x40, 0xb4, 0xa, 0x43, 0x24, 0x62, 0x4b,
    0xfc, 0x2, 0x40, 0x24,

    /* U+0035 "5" */
    0x3f, 0x83, 0x0, 0x30, 0x3, 0xb0, 0x0, 0xc0,
    0xc, 0x40, 0xc3, 0xf0,

    /* U+0036 "6" */
    0xf, 0x83, 0x0, 0x60, 0x6, 0xb4, 0x60, 0xc6,
    0xc, 0x30, 0xc1, 0xf4,

    /* U+0037 "7" */
    0x7f, 0xc0, 0x18, 0x3, 0x0, 0x60, 0x9, 0x0,
    0xc0, 0xc, 0x0, 0xc0,

    /* U+0038 "8" */
    0x1f, 0x3, 0x8, 0x30, 0x82, 0xa0, 0x22, 0x45,
    0xc, 0x50, 0xc2, 0xf4,

    /* U+0039 "9" */
    0x2e, 0x6, 0x18, 0x90, 0xc6, 0xc, 0x29, 0xc0,
    0xc, 0x2, 0x43, 0xe0,

    /* U+003A ":" */
    0x30, 0x40, 0x0, 0x10, 0xc0,

    /* U+003B ";" */
    0x30, 0x40, 0x0, 0x10, 0xc2, 0x14, 0x0,

    /* U+003C "<" */
    0x0, 0x0, 0x68, 0x78, 0x7, 0x80, 0x6, 0x80,
    0x0,

    /* U+003D "=" */
    0x6a, 0x80, 0x0, 0x6a, 0x80,

    /* U+003E ">" */
    0x0, 0x7, 0x80, 0x6, 0x80, 0x68, 0x78, 0x0,
    0x0,

    /* U+003F "?" */
    0x3e, 0x0, 0xc0, 0x30, 0x24, 0xc, 0x1, 0x0,
    0x40, 0x70,

    /* U+0040 "@" */
    0x2, 0xa9, 0x2, 0x0, 0x92, 0xa, 0x48, 0x88,
    0x52, 0x52, 0x20, 0x94, 0x8c, 0x55, 0x29, 0xa0,
    0x80, 0x0, 0x18, 0x0, 0x1, 0xa8, 0x0,

    /* U+0041 "A" */
    0xa, 0x0, 0x3c, 0x1, 0xa4, 0x9, 0x20, 0x30,
    0xc0, 0xff, 0x45, 0x6, 0x30, 0xc,

    /* U+0042 "B" */
    0xff, 0xc, 0x18, 0xc1, 0x8f, 0xf0, 0xc0, 0xcc,
    0xc, 0xc0, 0xcf, 0xf0,

    /* U+0043 "C" */
    0xb, 0xd0, 0xd0, 0x43, 0x0, 0x18, 0x0, 0x60,
    0x0, 0xc0, 0x3, 0x41, 0x2, 0xf4,

    /* U+0044 "D" */
    0xfe, 0xc, 0x18, 0xc0, 0xcc, 0x9, 0xc0, 0x9c,
    0xc, 0xc2, 0x8f, 0xe0,

    /* U+0045 "E" */
    0xff, 0x30, 0xc, 0x3, 0xf8, 0xc0, 0x30, 0xc,
    0x3, 0xfd,

    /* U+0046 "F" */
    0xff, 0x30, 0xc, 0x3, 0x0, 0xfe, 0x30, 0xc,
    0x3, 0x0,

    /* U+0047 "G" */
    0xb, 0xd0, 0xd0, 0x43, 0x0, 0x18, 0x0, 0x60,
    0xf0, 0xc0, 0x83, 0x43, 0x2, 0xf8,

    /* U+0048 "H" */
    0xc0, 0x9c, 0x9, 0xc0, 0x9f, 0xfd, 0xc0, 0x9c,
    0x9, 0xc0, 0x9c, 0x9,

    /* U+0049 "I" */
    0xff, 0xff,

    /* U+004A "J" */
    0x2, 0x40, 0x90, 0x24, 0x9, 0x2, 0x40, 0x94,
    0x30, 0xf8,

    /* U+004B "K" */
    0xc1, 0x8c, 0x30, 0xcc, 0xe, 0xc0, 0xe6, 0xc,
    0x30, 0xc1, 0x8c, 0xc,

    /* U+004C "L" */
    0xc0, 0x30, 0xc, 0x3, 0x0, 0xc0, 0x30, 0xc,
    0x3, 0xfc,

    /* U+004D "M" */
    0xd0, 0x33, 0x81, 0xce, 0xa, 0x39, 0x28, 0xc9,
    0x73, 0x28, 0xcc, 0x63, 0x30, 0xc,

    /* U+004E "N" */
    0xc0, 0x8e, 0x8, 0xe4, 0x8c, 0xc8, 0xc9, 0x8c,
    0x38, 0xc2, 0xcc, 0xc,

    /* U+004F "O" */
    0xb, 0xd0, 0xd0, 0xd3, 0x1, 0x98, 0x3, 0x60,
    0xc, 0xc0, 0x63, 0x43, 0x2, 0xf4,

    /* U+0050 "P" */
    0xff, 0x30, 0x6c, 0xf, 0x6, 0xfe, 0x30, 0xc,
    0x3, 0x0,

    /* U+0051 "Q" */
    0xb, 0xd0, 0xd0, 0xd3, 0x1, 0x98, 0x3, 0x60,
    0xc, 0xc0, 0x63, 0x43, 0x2, 0xf4, 0x1, 0x80,
    0x1, 0xe0,

    /* U+0052 "R" */
    0xff, 0xc, 0x18, 0xc0, 0xcc, 0x18, 0xfe, 0xc,
    0x60, 0xc3, 0xc, 0x1c,

    /* U+0053 "S" */
    0x1f, 0x83, 0x4, 0x30, 0x1, 0xd0, 0x2, 0xc0,
    0x9, 0x10, 0x92, 0xf8,

    /* U+0054 "T" */
    0xbf, 0xe0, 0x50, 0x5, 0x0, 0x50, 0x5, 0x0,
    0x50, 0x5, 0x0, 0x50,

    /* U+0055 "U" */
    0x30, 0x20, 0xc0, 0x83, 0x2, 0xc, 0x8, 0x30,
    0x20, 0xc0, 0x82, 0x43, 0x3, 0xf4,

    /* U+0056 "V" */
    0xc0, 0x69, 0x9, 0x60, 0xc3, 0x8, 0x21, 0x41,
    0x60, 0xf, 0x0, 0xe0,

    /* U+0057 "W" */
    0x90, 0xc1, 0x54, 0x70, 0x86, 0x29, 0x30, 0xc8,
    0x8c, 0x32, 0x32, 0x9, 0x49, 0x41, 0xc1, 0xd0,
    0x70, 0x30,

    /* U+0058 "X" */
    0x60, 0xc3, 0x18, 0x1a, 0x0, 0xe0, 0xe, 0x2,
    0x64, 0x30, 0xc9, 0x9,

    /* U+0059 "Y" */
    0x30, 0x30, 0x61, 0x80, 0xc8, 0x1, 0xa0, 0x3,
    0x40, 0xc, 0x0, 0x30, 0x0, 0xc0,

    /* U+005A "Z" */
    0x3f, 0xd0, 0xc, 0x2, 0x40, 0x30, 0xc, 0x1,
    0x80, 0x30, 0x7, 0xfd,

    /* U+005B "[" */
    0xe2, 0x8, 0x20, 0x82, 0x8, 0x20, 0x82, 0xa,
    0x0,

    /* U+005C "\\" */
    0x80, 0x40, 0x20, 0x20, 0x20, 0x14, 0x8, 0x8,
    0x4, 0x2, 0x1,

    /* U+005D "]" */
    0x64, 0x51, 0x45, 0x14, 0x51, 0x45, 0x14, 0x56,
    0x40,

    /* U+005E "^" */
    0x4, 0x3, 0x81, 0x60, 0x85, 0x20, 0x80,

    /* U+005F "_" */
    0xaa, 0x90,

    /* U+0060 "`" */
    0x1, 0xc1, 0x40,

    /* U+0061 "a" */
    0x2f, 0x0, 0x20, 0x6c, 0xc3, 0x61, 0xcf, 0xb0,

    /* U+0062 "b" */
    0x30, 0x3, 0x0, 0x3b, 0x83, 0x9, 0x30, 0x63,
    0x6, 0x30, 0x93, 0xb8,

    /* U+0063 "c" */
    0x1f, 0x4c, 0x6, 0x1, 0x80, 0x30, 0x7, 0xd0,

    /* U+0064 "d" */
    0x0, 0x90, 0x9, 0x1f, 0xd3, 0x9, 0x60, 0x96,
    0x9, 0x30, 0xd2, 0xe9,

    /* U+0065 "e" */
    0x1f, 0x42, 0xc, 0x7a, 0xc6, 0x0, 0x30, 0x1,
    0xf8,

    /* U+0066 "f" */
    0x1d, 0x30, 0x30, 0xbc, 0x30, 0x30, 0x30, 0x30,
    0x30,

    /* U+0067 "g" */
    0x2f, 0xd2, 0x8, 0x31, 0x82, 0xa0, 0x20, 0x3,
    0xf8, 0x50, 0x92, 0xa8,

    /* U+0068 "h" */
    0x30, 0x3, 0x0, 0x3b, 0x83, 0xc, 0x30, 0x93,
    0x9, 0x30, 0x93, 0x9,

    /* U+0069 "i" */
    0x30, 0x33, 0x33, 0x33,

    /* U+006A "j" */
    0xc, 0x0, 0xc3, 0xc, 0x30, 0xc3, 0xd, 0xd0,

    /* U+006B "k" */
    0x30, 0x3, 0x0, 0x30, 0xc3, 0x30, 0x3a, 0x3,
    0xb0, 0x31, 0x83, 0xc,

    /* U+006C "l" */
    0x30, 0xc3, 0xc, 0x30, 0xc3, 0xd,

    /* U+006D "m" */
    0x2b, 0x9f, 0xc, 0x34, 0x93, 0xc, 0x24, 0xc3,
    0x9, 0x30, 0xc2, 0x4c, 0x30, 0x90,

    /* U+006E "n" */
    0x2b, 0x83, 0xc, 0x30, 0x93, 0x9, 0x30, 0x93,
    0x9,

    /* U+006F "o" */
    0x1f, 0x43, 0xc, 0x60, 0x56, 0x6, 0x30, 0xc1,
    0xf4,

    /* U+0070 "p" */
    0x3b, 0x83, 0x9, 0x30, 0x63, 0x6, 0x30, 0x93,
    0xb8, 0x30, 0x3, 0x0,

    /* U+0071 "q" */
    0x1f, 0x93, 0x9, 0x60, 0x96, 0x9, 0x30, 0xd2,
    0xed, 0x0, 0x90, 0x9,

    /* U+0072 "r" */
    0x2b, 0x34, 0x30, 0x30, 0x30, 0x30,

    /* U+0073 "s" */
    0x2e, 0x18, 0x3, 0x40, 0x1c, 0x2, 0x5f, 0x80,

    /* U+0074 "t" */
    0x10, 0x30, 0xbd, 0x30, 0x30, 0x30, 0x30, 0x1d,

    /* U+0075 "u" */
    0x30, 0xc3, 0xc, 0x30, 0xc3, 0xc, 0x30, 0xc2,
    0xe8,

    /* U+0076 "v" */
    0x80, 0xc6, 0x18, 0x32, 0x42, 0x30, 0x1a, 0x0,
    0xd0,

    /* U+0077 "w" */
    0x92, 0x82, 0x62, 0x85, 0x32, 0x88, 0x35, 0x5c,
    0x28, 0x28, 0x1c, 0x34,

    /* U+0078 "x" */
    0x62, 0x4c, 0xc1, 0xd0, 0x74, 0x33, 0x24, 0x60,

    /* U+0079 "y" */
    0x90, 0xc6, 0x18, 0x32, 0x2, 0x70, 0xe, 0x0,
    0xc0, 0xc, 0x7, 0x0,

    /* U+007A "z" */
    0x7f, 0x40, 0xc0, 0xc0, 0x60, 0x30, 0x2f, 0xd0,

    /* U+007B "{" */
    0x18, 0x20, 0x20, 0x20, 0x20, 0x60, 0x20, 0x20,
    0x20, 0x20, 0x18,

    /* U+007C "|" */
    0xaa, 0xaa, 0xaa,

    /* U+007D "}" */
    0x60, 0x82, 0x8, 0x20, 0x72, 0x8, 0x20, 0x86,
    0x0,

    /* U+007E "~" */
    0x28, 0x40, 0x28,

    /* U+2103 "℃" */
    0x24, 0xa, 0x48, 0x83, 0x48, 0x68, 0x90, 0x0,
    0xc, 0x0, 0x0, 0xc0, 0x0, 0xd, 0x0, 0x0,
    0x70, 0x0, 0x1, 0xf8,

    /* U+FF05 "％" */
    0x20, 0x4, 0x22, 0x8, 0x8, 0x81, 0x2, 0x22,
    0x28, 0x69, 0x61, 0x40, 0x88, 0x20, 0x42, 0x8,
    0x10, 0x68,

    /* U+FF0C "，" */
    0x1, 0xc2, 0x24, 0x0
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 36, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 0, .adv_w = 52, .box_w = 3, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 6, .adv_w = 76, .box_w = 4, .box_h = 4, .ofs_x = 0, .ofs_y = 4},
    {.bitmap_index = 10, .adv_w = 89, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 22, .adv_w = 89, .box_w = 5, .box_h = 11, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 36, .adv_w = 147, .box_w = 9, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 54, .adv_w = 109, .box_w = 7, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 68, .adv_w = 44, .box_w = 2, .box_h = 4, .ofs_x = 0, .ofs_y = 4},
    {.bitmap_index = 70, .adv_w = 54, .box_w = 3, .box_h = 12, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 79, .adv_w = 54, .box_w = 3, .box_h = 12, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 88, .adv_w = 75, .box_w = 5, .box_h = 4, .ofs_x = 0, .ofs_y = 4},
    {.bitmap_index = 93, .adv_w = 89, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 102, .adv_w = 44, .box_w = 3, .box_h = 5, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 106, .adv_w = 56, .box_w = 4, .box_h = 1, .ofs_x = 0, .ofs_y = 3},
    {.bitmap_index = 107, .adv_w = 44, .box_w = 3, .box_h = 2, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 109, .adv_w = 63, .box_w = 4, .box_h = 11, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 120, .adv_w = 89, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 132, .adv_w = 89, .box_w = 5, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 142, .adv_w = 89, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 154, .adv_w = 89, .box_w = 5, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 164, .adv_w = 89, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 176, .adv_w = 89, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 188, .adv_w = 89, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 200, .adv_w = 89, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 212, .adv_w = 89, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 224, .adv_w = 89, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 236, .adv_w = 44, .box_w = 3, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 241, .adv_w = 44, .box_w = 3, .box_h = 9, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 248, .adv_w = 89, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 257, .adv_w = 89, .box_w = 6, .box_h = 3, .ofs_x = 0, .ofs_y = 3},
    {.bitmap_index = 262, .adv_w = 89, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 271, .adv_w = 76, .box_w = 5, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 281, .adv_w = 151, .box_w = 9, .box_h = 10, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 304, .adv_w = 97, .box_w = 7, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 318, .adv_w = 105, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 330, .adv_w = 102, .box_w = 7, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 344, .adv_w = 110, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 356, .adv_w = 94, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 366, .adv_w = 88, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 376, .adv_w = 110, .box_w = 7, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 390, .adv_w = 116, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 402, .adv_w = 47, .box_w = 1, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 404, .adv_w = 86, .box_w = 5, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 414, .adv_w = 103, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 426, .adv_w = 87, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 436, .adv_w = 130, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 450, .adv_w = 116, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 462, .adv_w = 119, .box_w = 7, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 476, .adv_w = 101, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 486, .adv_w = 119, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 504, .adv_w = 102, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 516, .adv_w = 95, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 528, .adv_w = 96, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 540, .adv_w = 115, .box_w = 7, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 554, .adv_w = 92, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 566, .adv_w = 140, .box_w = 9, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 584, .adv_w = 92, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 596, .adv_w = 85, .box_w = 7, .box_h = 8, .ofs_x = -1, .ofs_y = 0},
    {.bitmap_index = 610, .adv_w = 96, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 622, .adv_w = 54, .box_w = 3, .box_h = 11, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 631, .adv_w = 63, .box_w = 4, .box_h = 11, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 642, .adv_w = 54, .box_w = 3, .box_h = 11, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 651, .adv_w = 89, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 3},
    {.bitmap_index = 658, .adv_w = 89, .box_w = 6, .box_h = 1, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 660, .adv_w = 97, .box_w = 3, .box_h = 4, .ofs_x = 1, .ofs_y = 6},
    {.bitmap_index = 663, .adv_w = 90, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 671, .adv_w = 99, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 683, .adv_w = 82, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 691, .adv_w = 99, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 703, .adv_w = 89, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 712, .adv_w = 52, .box_w = 4, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 721, .adv_w = 90, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 733, .adv_w = 97, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 745, .adv_w = 44, .box_w = 2, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 749, .adv_w = 44, .box_w = 3, .box_h = 10, .ofs_x = -1, .ofs_y = -2},
    {.bitmap_index = 757, .adv_w = 88, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 769, .adv_w = 45, .box_w = 3, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 775, .adv_w = 148, .box_w = 9, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 789, .adv_w = 98, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 798, .adv_w = 97, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 807, .adv_w = 99, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 819, .adv_w = 99, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 831, .adv_w = 62, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 837, .adv_w = 75, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 845, .adv_w = 60, .box_w = 4, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 853, .adv_w = 97, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 862, .adv_w = 83, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 871, .adv_w = 128, .box_w = 8, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 883, .adv_w = 80, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 891, .adv_w = 83, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 903, .adv_w = 76, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 911, .adv_w = 54, .box_w = 4, .box_h = 11, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 922, .adv_w = 43, .box_w = 1, .box_h = 12, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 925, .adv_w = 54, .box_w = 3, .box_h = 11, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 934, .adv_w = 89, .box_w = 6, .box_h = 2, .ofs_x = 0, .ofs_y = 3},
    {.bitmap_index = 937, .adv_w = 160, .box_w = 10, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 957, .adv_w = 160, .box_w = 9, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 975, .adv_w = 160, .box_w = 3, .box_h = 5, .ofs_x = 1, .ofs_y = -2}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/

static const uint16_t unicode_list_1[] = {
    0x0, 0xde02, 0xde09
};

/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 32, .range_length = 95, .glyph_id_start = 1,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    },
    {
        .range_start = 8451, .range_length = 56842, .glyph_id_start = 96,
        .unicode_list = unicode_list_1, .glyph_id_ofs_list = NULL, .list_length = 3, .type = LV_FONT_FMT_TXT_CMAP_SPARSE_TINY
    }
};

/*-----------------
 *    KERNING
 *----------------*/


/*Map glyph_ids to kern left classes*/
static const uint8_t kern_left_class_mapping[] =
{
    0, 0, 0, 1, 0, 0, 0, 0,
    1, 2, 0, 0, 0, 3, 4, 3,
    5, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 6, 6, 0, 0, 0,
    0, 0, 7, 8, 9, 10, 11, 12,
    13, 0, 0, 14, 15, 16, 0, 0,
    10, 17, 10, 18, 19, 20, 21, 22,
    23, 24, 25, 26, 2, 27, 0, 0,
    0, 0, 28, 29, 30, 0, 31, 32,
    33, 34, 0, 0, 35, 36, 34, 34,
    29, 29, 37, 38, 39, 40, 37, 41,
    42, 43, 44, 45, 2, 0, 0, 0,
    0, 0, 0
};

/*Map glyph_ids to kern right classes*/
static const uint8_t kern_right_class_mapping[] =
{
    0, 0, 1, 2, 0, 0, 0, 0,
    2, 0, 3, 4, 0, 5, 6, 7,
    8, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 9, 10, 0, 0, 0,
    11, 0, 12, 0, 13, 0, 0, 0,
    13, 0, 0, 14, 0, 0, 0, 0,
    13, 0, 13, 0, 15, 16, 17, 18,
    19, 20, 21, 22, 0, 23, 3, 0,
    0, 0, 24, 0, 25, 25, 25, 26,
    27, 0, 28, 29, 0, 0, 30, 30,
    25, 30, 25, 30, 31, 32, 33, 34,
    35, 36, 37, 38, 0, 0, 3, 0,
    0, 0, 0
};

/*Kern values between classes*/
static const int8_t kern_class_values[] =
{
    0, 0, 0, 0, -21, 0, -21, 0,
    0, 0, 0, -10, 0, -17, -2, 0,
    0, 0, 0, -2, 0, 0, 0, 0,
    -6, 0, 0, 0, 0, 0, -4, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -4, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 14, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -17, 0, -25,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -18, -4, -12, -6, 0,
    -17, 0, 0, 0, -3, 0, 0, 0,
    4, 0, 0, -9, 0, -6, -4, 0,
    -4, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, -4,
    -4, -9, 0, -4, -2, -5, -12, -4,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -5, 0, -2, 0, -3, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -8, -2, -15, 0, 0,
    0, 0, 0, 0, 0, 0, 0, -5,
    -6, 0, -2, 4, 4, 0, 0, 1,
    -4, 0, 0, 0, 0, 0, 0, 0,
    0, -9, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, -5, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -10, 0, -17,
    0, 0, 0, 0, 0, 0, -5, -1,
    -2, 0, 0, -10, -3, -3, 0, 0,
    -3, -2, -8, 4, 0, -2, 0, 0,
    0, 0, 4, -3, -1, -2, -1, -1,
    -2, 0, 0, 0, 0, -6, 0, 0,
    0, 0, 0, 0, 0, 0, 0, -3,
    -3, -5, 0, -1, -1, -1, -3, -1,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -2, 0, -3, -2, -2, -3, 0,
    0, 0, 0, 0, 0, -5, 0, 0,
    0, 0, 0, 0, -5, -2, -5, -4,
    -3, -1, -1, -1, -2, -2, 0, 0,
    0, 0, -4, 0, 0, 0, 0, -5,
    -2, -3, -2, 0, -3, 0, 0, 0,
    0, -7, 0, 0, 0, -4, 0, 0,
    0, -2, 0, -8, 0, -5, 0, -2,
    -1, -4, -4, -4, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -3, 0, -2, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, -2, 0, 0, 0,
    0, 0, 0, -5, 0, -2, 0, -6,
    -2, 0, 0, 0, 0, 0, -13, 0,
    -13, -13, 0, 0, 0, -7, -2, -25,
    -4, 0, 0, 0, 0, -5, 0, -6,
    0, -6, -3, 0, -5, 0, 0, -4,
    -4, -2, -3, -4, -3, -5, -3, -6,
    0, 0, 0, -5, 0, 0, 0, 0,
    0, 0, 0, -1, 0, 0, 0, -4,
    0, -3, -1, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, -5, 0, -5, 0, 0, 0,
    0, 0, 0, -8, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -4, 0, -8,
    0, -6, 0, 0, 0, 0, -2, -2,
    -4, 0, -2, -4, -3, -3, -2, 0,
    -3, 0, 0, 0, -2, 0, 0, 0,
    -2, 0, 0, -6, -3, -4, -3, -3,
    -4, -3, 0, -16, 0, -28, 0, -10,
    0, 0, 0, 0, -6, 0, -5, 0,
    -4, -22, -5, -14, -10, 0, -14, 0,
    -15, 0, -3, -3, -1, 0, 0, 0,
    0, -4, -2, -7, -6, 0, -7, 0,
    0, 0, 0, 0, -20, -6, -20, -14,
    0, 0, 0, -9, 0, -27, -2, -5,
    0, 0, 0, -5, -2, -15, 0, -8,
    -5, 0, -6, 0, 0, 0, -2, 0,
    0, 0, 0, -3, 0, -4, 0, 0,
    0, -2, 0, -6, 0, 0, 0, 0,
    0, -1, 0, -4, -3, -3, 0, 0,
    1, -1, -1, -2, 0, -1, -2, 0,
    -1, 0, 0, 0, 0, 0, 0, 0,
    0, -2, 0, -2, 0, 0, 0, -3,
    0, 2, 0, 0, 0, 0, 0, 0,
    0, -3, -3, -4, 0, 0, 0, 0,
    -3, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -5, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, -19, -13,
    -19, -17, -4, -4, 0, -8, -5, -23,
    -8, 0, 0, 0, 0, -4, -3, -10,
    0, -13, -12, -4, -13, 0, 0, -9,
    -11, -4, -9, -6, -7, -8, -6, -14,
    0, 0, 0, 0, -3, 0, -3, -6,
    0, 0, 0, -3, 0, -9, -2, 0,
    0, -1, 0, -2, -3, 0, 0, -1,
    0, 0, -2, 0, 0, 0, -1, 0,
    0, 0, 0, -2, 0, 0, 0, 0,
    0, 0, -12, -4, -12, -9, 0, 0,
    0, -3, -2, -13, -2, 0, -2, 1,
    0, 0, 0, -4, 0, -4, -3, 0,
    -4, 0, 0, -4, -3, 0, -6, -2,
    -2, -3, -2, -5, 0, 0, 0, 0,
    -6, -2, -6, -6, 0, 0, 0, 0,
    -1, -12, -1, 0, 0, 0, 0, 0,
    0, -1, 0, -3, 0, 0, -3, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -2, 0, -2, 0, -2, 0, -5,
    0, 0, 0, 0, 0, 0, -4, -1,
    -3, -4, -2, 0, 0, 0, 0, 0,
    0, -2, -2, -3, 0, 0, 0, 0,
    0, -3, -2, -3, -3, -2, -3, -3,
    0, 0, 0, 0, -16, -12, -16, -12,
    -5, -5, -2, -3, -3, -18, -3, -3,
    -2, 0, 0, 0, 0, -5, 0, -12,
    -8, 0, -11, 0, 0, -8, -8, -5,
    -6, -3, -5, -6, -3, -9, 0, 0,
    0, 0, 0, -7, 0, 0, 0, 0,
    0, -1, -4, -6, -6, 0, -2, -1,
    -1, 0, -3, -3, 0, -3, -4, -4,
    -3, 0, 0, 0, 0, -3, -5, -3,
    -3, -5, -3, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -16, -6, -10, -6, 0,
    -13, 0, 0, 0, 0, 0, 5, 0,
    13, 0, 0, 0, 0, -4, -2, 0,
    2, 0, 0, 0, 0, -10, 0, 0,
    0, 0, 0, 0, -3, 0, 0, 0,
    0, -5, 0, -3, -1, 0, -5, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -3, 0, 0, 0, 0, 0, 0,
    0, -6, 0, -5, -2, 1, -2, 0,
    0, 0, -3, 0, 0, 0, 0, -11,
    0, -4, 0, -1, -9, 0, -5, -3,
    0, -1, 0, 0, 0, 0, -1, -4,
    0, -1, -1, -4, -1, -1, 0, 0,
    0, 0, 0, -4, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -4, 0, -3,
    0, 0, -5, 0, 0, -2, -4, 0,
    -2, 0, 0, 0, 0, -2, 0, 1,
    1, 1, 1, 0, 0, 0, 0, -7,
    0, 1, 0, 0, 0, 0, -2, 0,
    0, -4, -4, -5, 0, -3, -2, 0,
    -5, 0, -4, -3, 0, -1, -2, 0,
    0, 0, 0, -2, 0, 0, 0, -2,
    0, 0, 2, 7, 8, 0, -9, -3,
    -9, -3, 0, 0, 4, 0, 0, 0,
    0, 8, 0, 12, 8, 5, 10, 0,
    11, -4, -2, 0, -3, 0, -2, 0,
    -1, 0, 0, 2, 0, -1, 0, -3,
    0, 0, 2, -6, 0, 0, 0, 8,
    0, 0, -7, 0, 0, 0, 0, -5,
    0, 0, 0, 0, -3, 0, 0, -3,
    -3, 0, 0, 0, 6, 0, 0, 0,
    0, -1, -1, 0, 2, -3, 0, 0,
    0, -7, 0, 0, 0, 0, 0, 0,
    -2, 0, 0, 0, 0, -5, 0, -2,
    0, 0, -3, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, -4,
    2, -8, 2, 0, 2, 2, -3, 0,
    0, 0, 0, -7, 0, 0, 0, 0,
    -2, 0, 0, -2, -4, 0, -2, 0,
    -2, 0, 0, -4, -3, 0, 0, -2,
    0, -2, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 1, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -5, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, -4,
    0, -3, 0, 0, -6, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, -10, -5, -10, -7, 4, 4,
    0, -3, 0, -10, 0, 0, 0, 0,
    0, 0, 0, -2, 2, -5, -2, 0,
    -2, 0, 0, 0, -1, 0, 0, 4,
    3, 0, 4, -1, 0, 0, 0, -9,
    0, 1, 0, 0, 0, 0, -2, 0,
    0, 0, 0, -5, 0, -2, 0, 0,
    -4, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -4, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 1, -5,
    1, 1, 2, 2, -5, 0, 0, 0,
    0, -3, 0, 0, 0, 0, -1, 0,
    0, -4, -3, 0, -2, 0, 0, 0,
    -2, -4, 0, 0, 0, -3, 0, 0,
    0, 0, 0, -3, -6, -2, -6, -4,
    0, 0, 0, -2, 0, -8, 0, -4,
    0, -2, 0, 0, -3, -2, 0, -4,
    -1, 0, 0, 0, -2, 0, 0, 0,
    0, 0, 0, 0, 0, -5, 0, 0,
    0, -3, -7, 0, -7, -2, 0, 0,
    0, -1, 0, -6, 0, -5, 0, -2,
    0, -3, -5, 0, 0, -2, -1, 0,
    0, 0, -2, 0, 0, 0, 0, 0,
    0, 0, 0, -4, -3, 0, 0, -4,
    1, -3, -2, 0, 0, 1, 0, 0,
    -2, 0, -1, -6, 0, -3, 0, -2,
    -6, 0, 0, -2, -4, 0, 0, 0,
    0, 0, 0, -5, 0, 0, 0, 0,
    -1, 0, 0, 0, 0, 0, -6, 0,
    -6, -3, 0, 0, 0, 0, 0, -8,
    0, -4, 0, -1, 0, -1, -2, 0,
    0, -4, -1, 0, 0, 0, -2, 0,
    0, 0, 0, 0, 0, -3, 0, -5,
    0, 0, 0, 0, 0, -3, 0, 0,
    0, 0, 0, 0, 0, 0, 0, -5,
    0, 0, 0, 0, -6, 0, 0, -5,
    -2, 0, -1, 0, 0, 0, 0, 0,
    -2, -1, 0, 0, -1, 0
};


/*Collect the kern class' data in one place*/
static const lv_font_fmt_txt_kern_classes_t kern_classes =
{
    .class_pair_values   = kern_class_values,
    .left_class_mapping  = kern_left_class_mapping,
    .right_class_mapping = kern_right_class_mapping,
    .left_class_cnt      = 45,
    .right_class_cnt     = 38,
};

/*--------------------
 *  ALL CUSTOM DATA
 *--------------------*/

#if LVGL_VERSION_MAJOR == 8
/*Store all the custom data of the font*/
static  lv_font_fmt_txt_glyph_cache_t cache;
#endif

#if LVGL_VERSION_MAJOR >= 8
static const lv_font_fmt_txt_dsc_t font_dsc = {
#else
static lv_font_fmt_txt_dsc_t font_dsc = {
#endif
    .glyph_bitmap = glyph_bitmap,
    .glyph_dsc = glyph_dsc,
    .cmaps = cmaps,
    .kern_dsc = &kern_classes,
    .kern_scale = 16,
    .cmap_num = 2,
    .bpp = 2,
    .kern_classes = 1,
    .bitmap_format = 0,
#if LVGL_VERSION_MAJOR == 8
    .cache = &cache
#endif

};



/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LVGL_VERSION_MAJOR >= 8
const lv_font_t my_font_source_han_10 = {
#else
lv_font_t my_font_source_han_10 = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 13,          /*The maximum line height required by the font*/
    .base_line = 3,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = -1,
    .underline_thickness = 1,
#endif
    .static_bitmap = 0,
    .dsc = &font_dsc,          /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
#if LV_VERSION_CHECK(8, 2, 0) || LVGL_VERSION_MAJOR >= 9
    .fallback = NULL,
#endif
    .user_data = NULL,
};



#endif /*#if MY_FONT_SOURCE_HAN_10*/
