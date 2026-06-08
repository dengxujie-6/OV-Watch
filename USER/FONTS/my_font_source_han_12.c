/*******************************************************************************
 * Size: 12 px
 * Bpp: 2
 * Opts: --bpp 2 --size 12 --no-compress --stride 1 --align 1 --font NotoSansSC-Regular.ttf --symbols ℃％/， --range 32-127 --format lvgl -o my_font_source_han_12.c
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



#ifndef MY_FONT_SOURCE_HAN_12
#define MY_FONT_SOURCE_HAN_12 1
#endif

#if MY_FONT_SOURCE_HAN_12

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0020 " " */

    /* U+0021 "!" */
    0x99, 0x99, 0x95, 0x45, 0xa0,

    /* U+0022 "\"" */
    0xca, 0xca, 0xc5, 0x40,

    /* U+0023 "#" */
    0x8, 0x50, 0x21, 0x43, 0xff, 0x2, 0x20, 0x14,
    0x81, 0xff, 0xc2, 0x8, 0x8, 0x20, 0x20, 0x40,

    /* U+0024 "$" */
    0x1, 0x0, 0x30, 0xf, 0xc3, 0x45, 0x30, 0x2,
    0xc0, 0x7, 0x80, 0xa, 0x0, 0x72, 0xa, 0x1f,
    0x80, 0x30, 0x1, 0x0,

    /* U+0025 "%" */
    0x2a, 0x2, 0x1, 0x49, 0x8, 0x9, 0x14, 0x80,
    0x18, 0x95, 0x68, 0x2a, 0x22, 0x24, 0x1, 0x54,
    0x60, 0x8, 0x51, 0x80, 0x50, 0x85, 0x2, 0x2,
    0xa0,

    /* U+0026 "&" */
    0xb, 0xc0, 0x18, 0x90, 0x18, 0xc0, 0xf, 0x40,
    0x1e, 0x6, 0x73, 0x49, 0xa0, 0xec, 0x70, 0x78,
    0x2f, 0x8a,

    /* U+0027 "'" */
    0xcc, 0xc4,

    /* U+0028 "(" */
    0x0, 0x93, 0x18, 0x93, 0xc, 0x30, 0xc2, 0x5,
    0xc, 0x20, 0x0,

    /* U+0029 ")" */
    0x1, 0x83, 0x5, 0x8, 0x30, 0xc3, 0xc, 0x21,
    0x48, 0x20, 0x0,

    /* U+002A "*" */
    0x8, 0xb, 0xa0, 0xe0, 0x58, 0x0, 0x0,

    /* U+002B "+" */
    0x1, 0x0, 0xc, 0x0, 0x30, 0x1f, 0xfc, 0x3,
    0x0, 0xc, 0x0, 0x30, 0x0,

    /* U+002C "," */
    0x20, 0xe1, 0x8c, 0x0,

    /* U+002D "-" */
    0x7e,

    /* U+002E "." */
    0x20, 0xd0,

    /* U+002F "/" */
    0x1, 0x40, 0x80, 0x30, 0x14, 0x8, 0x3, 0x0,
    0x80, 0x90, 0x30, 0x8, 0x5, 0x2, 0x0,

    /* U+0030 "0" */
    0xf, 0x80, 0xd2, 0x83, 0x3, 0x18, 0xc, 0x60,
    0x31, 0x80, 0xc3, 0x3, 0xd, 0x28, 0xf, 0x80,

    /* U+0031 "1" */
    0x2c, 0x7, 0x0, 0xc0, 0x30, 0xc, 0x3, 0x0,
    0xc0, 0x30, 0xff, 0xc0,

    /* U+0032 "2" */
    0x2f, 0x80, 0x83, 0x40, 0x6, 0x0, 0x28, 0x0,
    0xc0, 0xa, 0x0, 0xa0, 0xa, 0x0, 0x7f, 0xf0,

    /* U+0033 "3" */
    0x2f, 0x82, 0xa, 0x0, 0x60, 0xd, 0xf, 0x80,
    0xa, 0x0, 0x35, 0xb, 0x2f, 0x80,

    /* U+0034 "4" */
    0x1, 0xc0, 0xf, 0x0, 0xac, 0x3, 0x30, 0x34,
    0xc1, 0x83, 0xb, 0xff, 0x40, 0x30, 0x0, 0xc0,

    /* U+0035 "5" */
    0x2f, 0xe0, 0xc0, 0x3, 0x0, 0xf, 0xe0, 0x10,
    0xa0, 0x0, 0xc0, 0x3, 0x14, 0x28, 0x2f, 0x80,

    /* U+0036 "6" */
    0xb, 0xd0, 0xa0, 0x43, 0x0, 0x1e, 0xf0, 0x74,
    0x71, 0xc0, 0xc3, 0x3, 0x9, 0x1c, 0xb, 0xc0,

    /* U+0037 "7" */
    0x7f, 0xf0, 0x1, 0x80, 0xc, 0x0, 0x90, 0x3,
    0x0, 0xc, 0x0, 0x60, 0x2, 0x80, 0xa, 0x0,

    /* U+0038 "8" */
    0xf, 0xc0, 0xc1, 0x83, 0x3, 0xa, 0x14, 0x1f,
    0xc0, 0xc2, 0x86, 0x3, 0x1c, 0x1c, 0x1f, 0xc0,

    /* U+0039 "9" */
    0x1f, 0x81, 0xc2, 0x46, 0x3, 0x1c, 0x1c, 0x2f,
    0x70, 0x0, 0xc0, 0x6, 0x4, 0x34, 0x2f, 0x40,

    /* U+003A ":" */
    0x34, 0x80, 0x0, 0x0, 0x83, 0x40,

    /* U+003B ";" */
    0x34, 0x80, 0x0, 0x0, 0x83, 0x86, 0x30, 0x0,

    /* U+003C "<" */
    0x0, 0x0, 0x7, 0xc2, 0xe0, 0x2d, 0x0, 0xb,
    0x80, 0x1, 0xc0, 0x0, 0x0,

    /* U+003D "=" */
    0x7f, 0xf0, 0x0, 0x0, 0x0, 0x1f, 0xfc,

    /* U+003E ">" */
    0x0, 0x1, 0xe0, 0x0, 0x6d, 0x0, 0x2c, 0x1b,
    0x82, 0x90, 0x0, 0x0, 0x0,

    /* U+003F "?" */
    0x2f, 0x41, 0x1c, 0x0, 0xc0, 0x24, 0x6, 0x0,
    0xc0, 0x0, 0x0, 0x80, 0xd, 0x0,

    /* U+0040 "@" */
    0x0, 0xbf, 0x40, 0x18, 0x1, 0x81, 0x80, 0x2,
    0x4c, 0x1e, 0x82, 0x20, 0xc6, 0x9, 0x49, 0x14,
    0x66, 0x24, 0x92, 0x8, 0x38, 0xf4, 0x30, 0x0,
    0x0, 0x34, 0x4, 0x0, 0x2f, 0x90, 0x0,

    /* U+0041 "A" */
    0x3, 0x40, 0x6, 0xc0, 0xc, 0xc0, 0xc, 0x90,
    0x18, 0x70, 0x3f, 0xf0, 0x30, 0x24, 0x60, 0x1c,
    0xd0, 0xc,

    /* U+0042 "B" */
    0xff, 0x83, 0x42, 0x8d, 0x7, 0x34, 0x28, 0xff,
    0xc3, 0x41, 0xcd, 0x3, 0x74, 0x1c, 0xff, 0xd0,

    /* U+0043 "C" */
    0x7, 0xf4, 0x1d, 0x8, 0x34, 0x0, 0x30, 0x0,
    0x70, 0x0, 0x30, 0x0, 0x34, 0x0, 0x1d, 0x8,
    0x7, 0xf4,

    /* U+0044 "D" */
    0xff, 0x83, 0x42, 0x8d, 0x3, 0x34, 0x9, 0xd0,
    0x2b, 0x40, 0x9d, 0x3, 0x34, 0x28, 0xff, 0x80,

    /* U+0045 "E" */
    0xff, 0xdd, 0x0, 0xd0, 0xd, 0x0, 0xff, 0x8d,
    0x0, 0xd0, 0xd, 0x0, 0xff, 0xd0,

    /* U+0046 "F" */
    0xff, 0xdd, 0x0, 0xd0, 0xd, 0x0, 0xff, 0x8d,
    0x0, 0xd0, 0xd, 0x0, 0xd0, 0x0,

    /* U+0047 "G" */
    0x7, 0xf4, 0x1d, 0x8, 0x34, 0x0, 0x30, 0x0,
    0x70, 0x7d, 0x30, 0x9, 0x34, 0x9, 0x1d, 0xd,
    0x7, 0xf8,

    /* U+0048 "H" */
    0xd0, 0x2b, 0x40, 0xad, 0x2, 0xb4, 0xa, 0xff,
    0xfb, 0x40, 0xad, 0x2, 0xb4, 0xa, 0xd0, 0x28,

    /* U+0049 "I" */
    0xdd, 0xdd, 0xdd, 0xdd, 0xd0,

    /* U+004A "J" */
    0x0, 0xd0, 0xd, 0x0, 0xd0, 0xd, 0x0, 0xd0,
    0xd, 0x0, 0xc6, 0x1c, 0x2f, 0x40,

    /* U+004B "K" */
    0xd0, 0x73, 0x43, 0xd, 0x34, 0x37, 0x80, 0xfb,
    0x3, 0xca, 0xd, 0xd, 0x34, 0x1c, 0xd0, 0x34,

    /* U+004C "L" */
    0xd0, 0xd, 0x0, 0xd0, 0xd, 0x0, 0xd0, 0xd,
    0x0, 0xd0, 0xd, 0x0, 0xff, 0xc0,

    /* U+004D "M" */
    0xe0, 0xe, 0xf0, 0x1e, 0xe4, 0x2a, 0xdc, 0x36,
    0xcc, 0x6a, 0xc5, 0x8a, 0xc3, 0xca, 0xc2, 0x4a,
    0xc0, 0xa,

    /* U+004E "N" */
    0xe0, 0x27, 0xc0, 0x9d, 0xc2, 0x73, 0x49, 0xd3,
    0x27, 0x49, 0x9d, 0xe, 0x74, 0x2d, 0xd0, 0x34,

    /* U+004F "O" */
    0x7, 0xf4, 0x7, 0x7, 0x43, 0x0, 0x70, 0xc0,
    0xc, 0x70, 0x3, 0xc, 0x0, 0xc3, 0x40, 0x70,
    0x70, 0x74, 0x7, 0xf4, 0x0,

    /* U+0050 "P" */
    0xff, 0x83, 0x41, 0xcd, 0x3, 0x34, 0x2c, 0xff,
    0x83, 0x40, 0xd, 0x0, 0x34, 0x0, 0xd0, 0x0,

    /* U+0051 "Q" */
    0x7, 0xf4, 0x7, 0x7, 0x43, 0x0, 0x70, 0xc0,
    0xc, 0x70, 0x3, 0xc, 0x0, 0xc3, 0x0, 0x70,
    0x70, 0x34, 0x7, 0xf4, 0x0, 0x38, 0x0, 0x2,
    0xf4,

    /* U+0052 "R" */
    0xff, 0xc3, 0x41, 0xcd, 0x3, 0x34, 0x2c, 0xff,
    0x83, 0x4d, 0xd, 0x1c, 0x34, 0x34, 0xd0, 0x30,

    /* U+0053 "S" */
    0xf, 0xe0, 0xd0, 0x83, 0x0, 0xb, 0x40, 0x7,
    0xd0, 0x1, 0xd0, 0x2, 0x8d, 0xd, 0x1f, 0xe0,

    /* U+0054 "T" */
    0xbf, 0xfc, 0xc, 0x0, 0x30, 0x0, 0xc0, 0x3,
    0x0, 0xc, 0x0, 0x30, 0x0, 0xc0, 0x3, 0x0,

    /* U+0055 "U" */
    0xd0, 0x27, 0x40, 0x9d, 0x2, 0x74, 0x9, 0xd0,
    0x27, 0x40, 0x99, 0x2, 0x5c, 0x1c, 0x1f, 0xd0,

    /* U+0056 "V" */
    0xd0, 0x1e, 0x80, 0x93, 0x3, 0xc, 0xc, 0x28,
    0xa0, 0x73, 0x0, 0xcc, 0x2, 0xe0, 0x7, 0x40,

    /* U+0057 "W" */
    0xa0, 0x70, 0x31, 0x82, 0xc0, 0xc3, 0xe, 0x46,
    0xc, 0x36, 0x28, 0x35, 0x8c, 0x90, 0x99, 0x33,
    0x1, 0xb0, 0x9c, 0x3, 0xc1, 0xe0, 0xe, 0x3,
    0x80,

    /* U+0058 "X" */
    0x70, 0x30, 0xd1, 0x81, 0xcc, 0x2, 0xe0, 0x7,
    0x40, 0x3a, 0x1, 0x8d, 0xc, 0x1c, 0xa0, 0x34,

    /* U+0059 "Y" */
    0x34, 0xc, 0x1c, 0x18, 0xd, 0x30, 0x6, 0x60,
    0x3, 0xc0, 0x1, 0xc0, 0x1, 0x80, 0x1, 0x80,
    0x1, 0x80,

    /* U+005A "Z" */
    0x3f, 0xf8, 0x0, 0xc0, 0x9, 0x0, 0x70, 0x3,
    0x0, 0x28, 0x1, 0xc0, 0xd, 0x0, 0x7f, 0xf8,

    /* U+005B "[" */
    0xa6, 0x8, 0x20, 0x82, 0x8, 0x20, 0x82, 0x8,
    0x29,

    /* U+005C "\\" */
    0x80, 0x14, 0x2, 0x0, 0x80, 0x14, 0x2, 0x0,
    0xc0, 0x20, 0x6, 0x0, 0xc0, 0x20, 0x5,

    /* U+005D "]" */
    0x6c, 0x30, 0xc3, 0xc, 0x30, 0xc3, 0xc, 0x30,
    0xda,

    /* U+005E "^" */
    0x1, 0x0, 0xb0, 0xd, 0x81, 0x8c, 0x24, 0x93,
    0x6,

    /* U+005F "_" */
    0xbf, 0xf4,

    /* U+0060 "`" */
    0x20, 0x28, 0x8,

    /* U+0061 "a" */
    0x1f, 0xc1, 0xa, 0x0, 0xb1, 0xeb, 0x30, 0x73,
    0xb, 0x2f, 0x70,

    /* U+0062 "b" */
    0xc0, 0xc, 0x0, 0xc0, 0xd, 0xf8, 0xe0, 0xdc,
    0x6, 0xc0, 0x7c, 0x6, 0xd0, 0xdd, 0xf4,

    /* U+0063 "c" */
    0xb, 0xd3, 0x80, 0x70, 0x7, 0x0, 0x70, 0x3,
    0x81, 0xf, 0xd0,

    /* U+0064 "d" */
    0x0, 0x34, 0x0, 0xd0, 0x3, 0x43, 0xed, 0x38,
    0x34, 0xc0, 0xd7, 0x3, 0x5c, 0xd, 0x34, 0x74,
    0x7e, 0x90,

    /* U+0065 "e" */
    0xb, 0xc0, 0xd1, 0xc3, 0x3, 0x1f, 0xfc, 0x70,
    0x0, 0xd0, 0x0, 0xbd, 0x0,

    /* U+0066 "f" */
    0xf, 0xa, 0x2, 0x42, 0xf8, 0x24, 0x9, 0x2,
    0x40, 0x90, 0x24, 0x9, 0x0,

    /* U+0067 "g" */
    0x1f, 0xf4, 0xc2, 0x43, 0x6, 0xd, 0x24, 0x2f,
    0x80, 0xc0, 0x2, 0xff, 0x8, 0xa, 0x70, 0x34,
    0xbf, 0x40,

    /* U+0068 "h" */
    0xc0, 0xc, 0x0, 0xc0, 0xd, 0xf8, 0xe0, 0xcc,
    0xd, 0xc0, 0xdc, 0xd, 0xc0, 0xdc, 0xd,

    /* U+0069 "i" */
    0x34, 0x40, 0xc, 0x30, 0xc3, 0xc, 0x30, 0xc0,

    /* U+006A "j" */
    0xd, 0x4, 0x0, 0xc, 0xc, 0xc, 0xc, 0xc,
    0xc, 0xc, 0xc, 0xc, 0x78,

    /* U+006B "k" */
    0xc0, 0xc, 0x0, 0xc0, 0xc, 0x1c, 0xc3, 0xc,
    0xd0, 0xfd, 0xe, 0x30, 0xc2, 0x8c, 0xc,

    /* U+006C "l" */
    0xcc, 0xcc, 0xcc, 0xcc, 0xcb,

    /* U+006D "m" */
    0xdf, 0x5f, 0x4e, 0xe, 0xc, 0xc0, 0xc0, 0xcc,
    0xc, 0xc, 0xc0, 0xc0, 0xcc, 0xc, 0xc, 0xc0,
    0xc0, 0xc0,

    /* U+006E "n" */
    0xdf, 0x8e, 0xc, 0xc0, 0xdc, 0xd, 0xc0, 0xdc,
    0xd, 0xc0, 0xd0,

    /* U+006F "o" */
    0xb, 0xd0, 0xd1, 0xc7, 0x2, 0x9c, 0x6, 0x70,
    0x28, 0xd0, 0xc0, 0xbd, 0x0,

    /* U+0070 "p" */
    0xdf, 0x8e, 0xd, 0xc0, 0x6c, 0x7, 0xc0, 0xae,
    0xd, 0xef, 0x4c, 0x0, 0xc0, 0xc, 0x0,

    /* U+0071 "q" */
    0xf, 0xa4, 0xe0, 0xd3, 0x3, 0x5c, 0xd, 0x70,
    0x34, 0xd1, 0xd1, 0xfb, 0x40, 0xd, 0x0, 0x34,
    0x0, 0xd0,

    /* U+0072 "r" */
    0x0, 0xde, 0xe0, 0xd0, 0xc0, 0xc0, 0xc0, 0xc0,

    /* U+0073 "s" */
    0x2f, 0x83, 0x0, 0x34, 0x0, 0xb4, 0x0, 0xc1,
    0xc, 0x2f, 0x40,

    /* U+0074 "t" */
    0x24, 0x9, 0xb, 0xf0, 0x90, 0x24, 0x9, 0x2,
    0x40, 0xa0, 0xf, 0x0,

    /* U+0075 "u" */
    0xc0, 0xcc, 0xc, 0xc0, 0xcc, 0xc, 0xc0, 0xcd,
    0x2c, 0x7d, 0xc0,

    /* U+0076 "v" */
    0x90, 0x31, 0xc1, 0x83, 0xd, 0x9, 0x30, 0x19,
    0x80, 0x39, 0x0, 0xb0, 0x0,

    /* U+0077 "w" */
    0xa0, 0xe0, 0xc7, 0xf, 0xc, 0x31, 0xb1, 0x83,
    0x66, 0x64, 0x27, 0x1b, 0x1, 0xb0, 0xf0, 0xe,
    0xe, 0x0,

    /* U+0078 "x" */
    0x70, 0xd3, 0x5c, 0xf, 0x0, 0xf0, 0x1b, 0x43,
    0x1c, 0xa0, 0x90,

    /* U+0079 "y" */
    0x90, 0x31, 0xc1, 0x83, 0x9, 0x9, 0x30, 0xd,
    0x80, 0x39, 0x0, 0x70, 0x1, 0x80, 0xc, 0x1,
    0xd0, 0x0,

    /* U+007A "z" */
    0x3f, 0xc0, 0x28, 0x3, 0x0, 0x90, 0x1c, 0x3,
    0x0, 0xbf, 0xd0,

    /* U+007B "{" */
    0xd, 0x14, 0x24, 0x14, 0x14, 0x24, 0x70, 0x24,
    0x14, 0x14, 0x14, 0x9,

    /* U+007C "|" */
    0xff, 0xff, 0xff, 0xf0,

    /* U+007D "}" */
    0x70, 0x18, 0x18, 0x18, 0x14, 0x18, 0xd, 0x18,
    0x14, 0x18, 0x18, 0x60,

    /* U+007E "~" */
    0x2d, 0x10, 0xb, 0x80,

    /* U+2103 "℃" */
    0x0, 0x0, 0x0, 0x69, 0x7, 0xf4, 0x82, 0x1d,
    0x8, 0x29, 0x34, 0x0, 0x0, 0x30, 0x0, 0x0,
    0x70, 0x0, 0x0, 0x30, 0x0, 0x0, 0x34, 0x0,
    0x0, 0x1c, 0x8, 0x0, 0xb, 0xf8, 0x0, 0x0,
    0x0,

    /* U+FF05 "％" */
    0x1e, 0x0, 0x80, 0x30, 0x82, 0x0, 0x30, 0xc5,
    0x0, 0x30, 0xc8, 0x50, 0x21, 0x95, 0xa8, 0xa,
    0x23, 0xc, 0x0, 0x53, 0xc, 0x0, 0x82, 0xc,
    0x2, 0x0, 0xf4,

    /* U+FF0C "，" */
    0x28, 0xb0, 0xcc, 0x0
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 43, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 0, .adv_w = 62, .box_w = 2, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 5, .adv_w = 91, .box_w = 4, .box_h = 4, .ofs_x = 1, .ofs_y = 6},
    {.bitmap_index = 9, .adv_w = 107, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 25, .adv_w = 107, .box_w = 6, .box_h = 13, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 45, .adv_w = 177, .box_w = 11, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 70, .adv_w = 131, .box_w = 8, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 88, .adv_w = 53, .box_w = 2, .box_h = 4, .ofs_x = 1, .ofs_y = 6},
    {.bitmap_index = 90, .adv_w = 65, .box_w = 3, .box_h = 14, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 101, .adv_w = 65, .box_w = 3, .box_h = 14, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 112, .adv_w = 90, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 5},
    {.bitmap_index = 119, .adv_w = 107, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 132, .adv_w = 53, .box_w = 3, .box_h = 5, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 136, .adv_w = 67, .box_w = 4, .box_h = 1, .ofs_x = 0, .ofs_y = 3},
    {.bitmap_index = 137, .adv_w = 53, .box_w = 3, .box_h = 2, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 139, .adv_w = 75, .box_w = 5, .box_h = 12, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 154, .adv_w = 107, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 170, .adv_w = 107, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 182, .adv_w = 107, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 198, .adv_w = 107, .box_w = 6, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 212, .adv_w = 107, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 228, .adv_w = 107, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 244, .adv_w = 107, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 260, .adv_w = 107, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 276, .adv_w = 107, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 292, .adv_w = 107, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 308, .adv_w = 53, .box_w = 3, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 314, .adv_w = 53, .box_w = 3, .box_h = 10, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 322, .adv_w = 107, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 335, .adv_w = 107, .box_w = 7, .box_h = 4, .ofs_x = 0, .ofs_y = 3},
    {.bitmap_index = 342, .adv_w = 107, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 355, .adv_w = 91, .box_w = 6, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 369, .adv_w = 182, .box_w = 11, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 400, .adv_w = 117, .box_w = 8, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 418, .adv_w = 126, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 434, .adv_w = 122, .box_w = 8, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 452, .adv_w = 132, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 468, .adv_w = 113, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 482, .adv_w = 106, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 496, .adv_w = 132, .box_w = 8, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 514, .adv_w = 140, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 530, .adv_w = 56, .box_w = 2, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 535, .adv_w = 103, .box_w = 6, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 549, .adv_w = 124, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 565, .adv_w = 104, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 579, .adv_w = 156, .box_w = 8, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 597, .adv_w = 139, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 613, .adv_w = 142, .box_w = 9, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 634, .adv_w = 122, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 650, .adv_w = 142, .box_w = 9, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 675, .adv_w = 122, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 691, .adv_w = 114, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 707, .adv_w = 115, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 723, .adv_w = 138, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 739, .adv_w = 110, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 755, .adv_w = 169, .box_w = 11, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 780, .adv_w = 110, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 796, .adv_w = 102, .box_w = 8, .box_h = 9, .ofs_x = -1, .ofs_y = 0},
    {.bitmap_index = 814, .adv_w = 116, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 830, .adv_w = 65, .box_w = 3, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 839, .adv_w = 75, .box_w = 5, .box_h = 12, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 854, .adv_w = 65, .box_w = 3, .box_h = 12, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 863, .adv_w = 107, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 4},
    {.bitmap_index = 872, .adv_w = 107, .box_w = 7, .box_h = 1, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 874, .adv_w = 116, .box_w = 4, .box_h = 3, .ofs_x = 1, .ofs_y = 8},
    {.bitmap_index = 877, .adv_w = 108, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 888, .adv_w = 119, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 903, .adv_w = 98, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 914, .adv_w = 119, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 932, .adv_w = 106, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 945, .adv_w = 62, .box_w = 5, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 958, .adv_w = 108, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 976, .adv_w = 117, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 991, .adv_w = 53, .box_w = 3, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 999, .adv_w = 53, .box_w = 4, .box_h = 13, .ofs_x = -1, .ofs_y = -3},
    {.bitmap_index = 1012, .adv_w = 106, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1027, .adv_w = 55, .box_w = 2, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1032, .adv_w = 178, .box_w = 10, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1050, .adv_w = 117, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1061, .adv_w = 116, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1074, .adv_w = 119, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1089, .adv_w = 119, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 1107, .adv_w = 74, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1115, .adv_w = 90, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1126, .adv_w = 72, .box_w = 5, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1138, .adv_w = 117, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1149, .adv_w = 100, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1162, .adv_w = 154, .box_w = 10, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1180, .adv_w = 96, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1191, .adv_w = 100, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 1209, .adv_w = 91, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1220, .adv_w = 65, .box_w = 4, .box_h = 12, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 1232, .adv_w = 52, .box_w = 1, .box_h = 14, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1236, .adv_w = 65, .box_w = 4, .box_h = 12, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 1248, .adv_w = 107, .box_w = 7, .box_h = 2, .ofs_x = 0, .ofs_y = 4},
    {.bitmap_index = 1252, .adv_w = 192, .box_w = 12, .box_h = 11, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 1285, .adv_w = 192, .box_w = 12, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1312, .adv_w = 192, .box_w = 3, .box_h = 5, .ofs_x = 1, .ofs_y = -2}
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
    0, 0, 0, 0, -25, 0, -25, 0,
    0, 0, 0, -12, 0, -21, -3, 0,
    0, 0, 0, -3, 0, 0, 0, 0,
    -7, 0, 0, 0, 0, 0, -5, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -5, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 17, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -21, 0, -30,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -22, -5, -14, -8, 0,
    -20, 0, 0, 0, -3, 0, 0, 0,
    5, 0, 0, -10, 0, -8, -5, 0,
    -5, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, -5,
    -4, -11, 0, -5, -3, -6, -15, -5,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -6, 0, -2, 0, -3, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -9, -3, -18, 0, 0,
    0, 0, 0, 0, 0, 0, 0, -6,
    -7, 0, -3, 5, 5, 0, 0, 1,
    -5, 0, 0, 0, 0, 0, 0, 0,
    0, -11, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, -6, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -12, 0, -21,
    0, 0, 0, 0, 0, 0, -6, -2,
    -3, 0, 0, -12, -4, -3, 0, 0,
    -3, -2, -9, 4, 0, -3, 0, 0,
    0, 0, 4, -3, -2, -2, -1, -1,
    -2, 0, 0, 0, 0, -7, 0, 0,
    0, 0, 0, 0, 0, 0, 0, -4,
    -3, -6, 0, -2, -1, -1, -3, -1,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -3, 0, -3, -3, -3, -3, 0,
    0, 0, 0, 0, 0, -6, 0, 0,
    0, 0, 0, 0, -7, -3, -6, -4,
    -3, -1, -1, -1, -2, -3, 0, 0,
    0, 0, -5, 0, 0, 0, 0, -6,
    -3, -3, -3, 0, -3, 0, 0, 0,
    0, -8, 0, 0, 0, -4, 0, 0,
    0, -3, 0, -9, 0, -6, 0, -3,
    -2, -4, -5, -5, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -3, 0, -2, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, -3, 0, 0, 0,
    0, 0, 0, -6, 0, -3, 0, -7,
    -3, 0, 0, 0, 0, 0, -16, 0,
    -16, -16, 0, 0, 0, -9, -3, -30,
    -5, 0, 0, 0, 0, -6, 0, -7,
    0, -8, -3, 0, -6, 0, 0, -5,
    -5, -3, -4, -5, -4, -6, -4, -7,
    0, 0, 0, -6, 0, 0, 0, 0,
    0, 0, 0, -1, 0, 0, 0, -5,
    0, -3, -1, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, -6, 0, -6, 0, 0, 0,
    0, 0, 0, -9, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -5, 0, -9,
    0, -7, 0, 0, 0, 0, -2, -3,
    -5, 0, -2, -4, -3, -3, -3, 0,
    -4, 0, 0, 0, -2, 0, 0, 0,
    -3, 0, 0, -8, -4, -5, -4, -4,
    -5, -3, 0, -19, 0, -33, 0, -12,
    0, 0, 0, 0, -7, 0, -6, 0,
    -5, -26, -7, -17, -12, 0, -17, 0,
    -18, 0, -3, -3, -1, 0, 0, 0,
    0, -5, -3, -8, -8, 0, -8, 0,
    0, 0, 0, 0, -25, -8, -25, -17,
    0, 0, 0, -11, 0, -32, -3, -6,
    0, 0, 0, -6, -3, -18, 0, -10,
    -6, 0, -7, 0, 0, 0, -3, 0,
    0, 0, 0, -3, 0, -5, 0, 0,
    0, -3, 0, -7, 0, 0, 0, 0,
    0, -1, 0, -4, -3, -3, 0, 1,
    1, -1, -1, -3, 0, -1, -3, 0,
    -1, 0, 0, 0, 0, 0, 0, 0,
    0, -2, 0, -2, 0, 0, 0, -4,
    0, 2, 0, 0, 0, 0, 0, 0,
    0, -3, -3, -5, 0, 0, 0, 0,
    -3, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -6, 0, 0, 0, 0,
    0, -1, 0, 0, 0, 0, -23, -16,
    -23, -20, -5, -5, 0, -9, -6, -28,
    -9, 0, 0, 0, 0, -5, -3, -12,
    0, -16, -15, -4, -16, 0, 0, -11,
    -13, -4, -11, -8, -8, -9, -8, -17,
    0, 0, 0, 0, -4, 0, -4, -7,
    0, 0, 0, -4, 0, -11, -3, 0,
    0, -1, 0, -3, -3, 0, 0, -1,
    0, 0, -3, 0, 0, 0, -1, 0,
    0, 0, 0, -2, 0, 0, 0, 0,
    0, 0, -14, -5, -14, -11, 0, 0,
    0, -3, -3, -16, -3, 0, -3, 2,
    0, 0, 0, -5, 0, -5, -4, 0,
    -5, 0, 0, -5, -3, 0, -7, -2,
    -2, -4, -2, -6, 0, 0, 0, 0,
    -8, -3, -8, -7, 0, 0, 0, 0,
    -2, -15, -2, 0, 0, 0, 0, 0,
    0, -2, 0, -4, 0, 0, -3, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -3, 0, -3, 0, -3, 0, -6,
    0, 0, 0, 0, 0, 0, -4, -1,
    -3, -5, -3, 0, 0, 0, 0, 0,
    0, -3, -2, -4, 0, 0, 0, 0,
    0, -4, -3, -4, -3, -3, -4, -3,
    0, 0, 0, 0, -20, -15, -20, -15,
    -6, -6, -2, -3, -3, -22, -4, -3,
    -3, 0, 0, 0, 0, -6, 0, -15,
    -9, 0, -13, 0, 0, -9, -9, -7,
    -8, -3, -6, -8, -3, -11, 0, 0,
    0, 0, 0, -8, 0, 0, 0, 0,
    0, -2, -5, -8, -7, 0, -3, -2,
    -2, 0, -3, -4, 0, -4, -5, -5,
    -4, 0, 0, 0, 0, -3, -6, -4,
    -4, -6, -4, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -19, -7, -12, -7, 0,
    -16, 0, 0, 0, 0, 0, 7, 0,
    15, 0, 0, 0, 0, -5, -3, 0,
    2, 0, 0, 0, 0, -12, 0, 0,
    0, 0, 0, 0, -3, 0, 0, 0,
    0, -6, 0, -4, -1, 0, -6, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -3, 0, 0, 0, 0, 0, 0,
    0, -7, 0, -6, -3, 1, -3, 0,
    0, 0, -3, 0, 0, 0, 0, -13,
    0, -5, 0, -1, -10, 0, -6, -4,
    0, -1, 0, 0, 0, 0, -1, -4,
    0, -1, -1, -4, -1, -2, 0, 0,
    0, 0, 0, -5, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -5, 0, -3,
    0, 0, -6, 0, 0, -3, -5, 0,
    -3, 0, 0, 0, 0, -3, 0, 1,
    1, 1, 1, 0, 0, 0, 0, -8,
    0, 2, 0, 0, 0, 0, -2, 0,
    0, -5, -5, -6, 0, -4, -3, 0,
    -6, 0, -5, -4, 0, -1, -3, 0,
    0, 0, 0, -3, 0, 1, 1, -2,
    1, 0, 2, 8, 10, 0, -11, -3,
    -11, -4, 0, 0, 5, 0, 0, 0,
    0, 9, 0, 14, 9, 7, 12, 0,
    13, -5, -3, 0, -4, 0, -3, 0,
    -1, 0, 0, 2, 0, -1, 0, -3,
    0, 0, 2, -8, 0, 0, 0, 10,
    0, 0, -8, 0, 0, 0, 0, -6,
    0, 0, 0, 0, -3, 0, 0, -4,
    -3, 0, 0, 0, 7, 0, 0, 0,
    0, -1, -1, 0, 3, -3, 0, 0,
    0, -8, 0, 0, 0, 0, 0, 0,
    -2, 0, 0, 0, 0, -6, 0, -3,
    0, 0, -4, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, -5,
    2, -9, 2, 0, 2, 2, -3, 0,
    0, 0, 0, -8, 0, 0, 0, 0,
    -3, 0, 0, -3, -4, 0, -3, 0,
    -3, 0, 0, -5, -3, 0, 0, -2,
    0, -2, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 1, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -6, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, -5,
    0, -3, 0, 0, -7, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, -12, -6, -12, -8, 5, 5,
    0, -3, 0, -12, 0, 0, 0, 0,
    0, 0, 0, -3, 2, -6, -3, 0,
    -3, 0, 0, 0, -1, 0, 0, 5,
    4, 0, 5, -1, 0, 0, 0, -11,
    0, 2, 0, 0, 0, 0, -3, 0,
    0, 0, 0, -6, 0, -3, 0, 0,
    -5, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -5, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 1, -6,
    1, 2, 2, 2, -6, 0, 0, 0,
    0, -3, 0, 0, 0, 0, -1, 0,
    0, -5, -3, 0, -3, 0, 0, 0,
    -3, -5, 0, 0, 0, -4, 0, 0,
    0, 0, 0, -3, -8, -2, -8, -5,
    0, 0, 0, -3, 0, -9, 0, -5,
    0, -2, 0, 0, -3, -3, 0, -5,
    -1, 0, 0, 0, -3, 0, 0, 0,
    0, 0, 0, 0, 0, -6, 0, 0,
    0, -3, -9, 0, -9, -2, 0, 0,
    0, -1, 0, -7, 0, -6, 0, -2,
    0, -3, -6, 0, 0, -3, -1, 0,
    0, 0, -3, 0, 0, 0, 0, 0,
    0, 0, 0, -4, -3, 0, 0, -5,
    1, -3, -2, 0, 0, 1, 0, 0,
    -3, 0, -1, -8, 0, -4, 0, -3,
    -8, 0, 0, -3, -4, 0, 0, 0,
    0, 0, 0, -6, 0, 0, 0, 0,
    -1, 0, 0, 0, 0, 0, -8, 0,
    -8, -4, 0, 0, 0, 0, 0, -9,
    0, -5, 0, -1, 0, -1, -2, 0,
    0, -5, -1, 0, 0, 0, -3, 0,
    0, 0, 0, 0, 0, -3, 0, -6,
    0, 0, 0, 0, 0, -4, 0, 0,
    0, 0, 0, 0, 0, 0, 0, -6,
    0, 0, 0, 0, -7, 0, 0, -6,
    -3, 0, -2, 0, 0, 0, 0, 0,
    -3, -1, 0, 0, -1, 0
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
const lv_font_t my_font_source_han_12 = {
#else
lv_font_t my_font_source_han_12 = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 14,          /*The maximum line height required by the font*/
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



#endif /*#if MY_FONT_SOURCE_HAN_12*/
