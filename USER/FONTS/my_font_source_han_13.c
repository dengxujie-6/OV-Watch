/*******************************************************************************
 * Size: 13 px
 * Bpp: 2
 * Opts: --bpp 2 --size 13 --no-compress --stride 1 --align 1 --font NotoSansSC-Regular.ttf --symbols ℃％/， --range 32-127 --format lvgl -o my_font_source_han_13.c
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



#ifndef MY_FONT_SOURCE_HAN_13
#define MY_FONT_SOURCE_HAN_13 1
#endif

#if MY_FONT_SOURCE_HAN_13

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0020 " " */

    /* U+0021 "!" */
    0x66, 0x66, 0x66, 0x16, 0xb0,

    /* U+0022 "\"" */
    0xd7, 0xd3, 0x93, 0x41,

    /* U+0023 "#" */
    0x8, 0x30, 0x30, 0x83, 0xff, 0x82, 0x14, 0x8,
    0x51, 0xff, 0xd1, 0x48, 0x9, 0x30, 0x20, 0xc0,

    /* U+0024 "$" */
    0x1, 0x0, 0xc, 0x0, 0xbd, 0xa, 0x4, 0x34,
    0x0, 0x74, 0x0, 0x7c, 0x0, 0x1c, 0x0, 0x34,
    0x80, 0xc1, 0xfd, 0x0, 0xc0, 0x1, 0x0,

    /* U+0025 "%" */
    0x2b, 0x0, 0x80, 0x61, 0x82, 0x0, 0x50, 0xc6,
    0x0, 0x61, 0x88, 0xa4, 0x1a, 0x16, 0x48, 0x0,
    0x23, 0x5, 0x0, 0x83, 0x5, 0x0, 0x82, 0x48,
    0x2, 0x0, 0xe8,

    /* U+0026 "&" */
    0x7, 0xd0, 0x3, 0x18, 0x0, 0xc6, 0x0, 0x3a,
    0x0, 0x1f, 0x2, 0x4c, 0xa0, 0xca, 0xa, 0xd1,
    0xc0, 0xf4, 0x1f, 0xd2, 0x40,

    /* U+0027 "'" */
    0xdd, 0x94,

    /* U+0028 "(" */
    0x0, 0x63, 0x8, 0x92, 0x4c, 0x30, 0x92, 0x46,
    0xc, 0x14, 0x10,

    /* U+0029 ")" */
    0x0, 0x30, 0x20, 0x18, 0xc, 0xc, 0xc, 0xc,
    0xc, 0xc, 0x18, 0x24, 0x30, 0x0,

    /* U+002A "*" */
    0x5, 0x2, 0xb8, 0xf, 0x1, 0x94, 0x0, 0x0,

    /* U+002B "+" */
    0x1, 0x0, 0xc, 0x0, 0x30, 0x1f, 0xfe, 0x3,
    0x0, 0xc, 0x0, 0x30, 0x0,

    /* U+002C "," */
    0x14, 0xf0, 0x8c, 0x0,

    /* U+002D "-" */
    0x7f,

    /* U+002E "." */
    0x24, 0xe0,

    /* U+002F "/" */
    0x0, 0x80, 0x50, 0x30, 0xc, 0x5, 0x2, 0x0,
    0xc0, 0x50, 0x20, 0xc, 0x6, 0x2, 0x0,

    /* U+0030 "0" */
    0xb, 0xc0, 0xe1, 0xc3, 0x3, 0x5c, 0xa, 0x70,
    0x29, 0xc0, 0xa3, 0x3, 0x4e, 0x1c, 0xb, 0xc0,

    /* U+0031 "1" */
    0x2d, 0x1, 0xd0, 0x9, 0x0, 0x90, 0x9, 0x0,
    0x90, 0x9, 0x0, 0x90, 0xff, 0xd0,

    /* U+0032 "2" */
    0x1f, 0xc0, 0x82, 0xc0, 0x3, 0x0, 0xc, 0x0,
    0xa0, 0x7, 0x0, 0x70, 0x7, 0x0, 0x7f, 0xf8,

    /* U+0033 "3" */
    0x1f, 0xd0, 0x81, 0xc0, 0x3, 0x0, 0x28, 0xb,
    0xc0, 0x1, 0xc0, 0x3, 0x58, 0x1d, 0x2f, 0xd0,

    /* U+0034 "4" */
    0x0, 0xe0, 0xe, 0x80, 0xaa, 0x6, 0x28, 0x30,
    0xa2, 0xff, 0xe0, 0xa, 0x0, 0x28, 0x0, 0xa0,

    /* U+0035 "5" */
    0x2f, 0xf0, 0x90, 0x3, 0x40, 0xf, 0xf4, 0x10,
    0x70, 0x0, 0xd0, 0x3, 0x58, 0x1c, 0x2f, 0xd0,

    /* U+0036 "6" */
    0xb, 0xe0, 0x70, 0x43, 0x0, 0xd, 0xf8, 0x74,
    0x34, 0xc0, 0xa3, 0x2, 0x8a, 0xd, 0xb, 0xd0,

    /* U+0037 "7" */
    0x7f, 0xf8, 0x0, 0xc0, 0x9, 0x0, 0x70, 0x2,
    0x40, 0xc, 0x0, 0x30, 0x1, 0xc0, 0x7, 0x0,

    /* U+0038 "8" */
    0xf, 0xd0, 0xd0, 0xc3, 0x3, 0x6, 0xc, 0xf,
    0xd0, 0xc1, 0xc6, 0x2, 0x8c, 0xd, 0x1f, 0xe0,

    /* U+0039 "9" */
    0x1f, 0x80, 0xc1, 0xc6, 0x3, 0x4c, 0xd, 0x1f,
    0xa8, 0x0, 0x90, 0x3, 0x4, 0x28, 0x2f, 0x80,

    /* U+003A ":" */
    0x38, 0x90, 0x0, 0x0, 0x93, 0x80,

    /* U+003B ";" */
    0x38, 0x90, 0x0, 0x0, 0x53, 0xc2, 0x30, 0x0,

    /* U+003C "<" */
    0x0, 0x4, 0x6, 0xd2, 0xe4, 0x1d, 0x0, 0xb,
    0x90, 0x1, 0xa0, 0x0, 0x0,

    /* U+003D "=" */
    0x7f, 0xf8, 0x0, 0x0, 0x0, 0x1f, 0xfe,

    /* U+003E ">" */
    0x0, 0x1, 0xe4, 0x0, 0x2e, 0x0, 0x1e, 0xb,
    0x91, 0xd0, 0x0, 0x0, 0x0,

    /* U+003F "?" */
    0x2f, 0x81, 0xd, 0x0, 0xd0, 0x1c, 0x3, 0x0,
    0x90, 0x0, 0x0, 0x90, 0xe, 0x0,

    /* U+0040 "@" */
    0x0, 0xbf, 0x90, 0x7, 0x40, 0x28, 0xc, 0x0,
    0xc, 0x20, 0x2e, 0x45, 0x30, 0xc3, 0x6, 0x21,
    0x83, 0x5, 0x21, 0x87, 0xc, 0x30, 0xb5, 0xe0,
    0x24, 0x0, 0x0, 0xe, 0x0, 0x0, 0x1, 0xbe,
    0x0,

    /* U+0041 "A" */
    0x3, 0xc0, 0x6, 0xc0, 0x9, 0xa0, 0xc, 0x30,
    0x1c, 0x30, 0x2f, 0xf8, 0x30, 0xc, 0x70, 0xd,
    0xd0, 0xa,

    /* U+0042 "B" */
    0xbf, 0xd2, 0x80, 0xca, 0x3, 0x68, 0x1c, 0xbf,
    0xe2, 0x80, 0xaa, 0x1, 0xe8, 0xa, 0xbf, 0xe0,

    /* U+0043 "C" */
    0x3, 0xf8, 0x1d, 0x9, 0x34, 0x0, 0x30, 0x0,
    0x30, 0x0, 0x30, 0x0, 0x34, 0x0, 0x1d, 0x6,
    0x7, 0xf8,

    /* U+0044 "D" */
    0xbf, 0x90, 0xa0, 0x74, 0xa0, 0x1c, 0xa0, 0xc,
    0xa0, 0xc, 0xa0, 0xc, 0xa0, 0x2c, 0xa0, 0x74,
    0xbf, 0x90,

    /* U+0045 "E" */
    0xbf, 0xfa, 0x0, 0xa0, 0xa, 0x0, 0xbf, 0xca,
    0x0, 0xa0, 0xa, 0x0, 0xbf, 0xf0,

    /* U+0046 "F" */
    0xbf, 0xfa, 0x0, 0xa0, 0xa, 0x0, 0xbf, 0xca,
    0x0, 0xa0, 0xa, 0x0, 0xa0, 0x0,

    /* U+0047 "G" */
    0x2, 0xfc, 0x1d, 0x5, 0x34, 0x0, 0x30, 0x0,
    0x30, 0x3f, 0x30, 0x3, 0x34, 0x3, 0x1d, 0x7,
    0x6, 0xfd,

    /* U+0048 "H" */
    0xa0, 0xc, 0xa0, 0xc, 0xa0, 0xc, 0xa0, 0xc,
    0xbf, 0xfc, 0xa0, 0xc, 0xa0, 0xc, 0xa0, 0xc,
    0xa0, 0xc,

    /* U+0049 "I" */
    0xaa, 0xaa, 0xaa, 0xaa, 0xa0,

    /* U+004A "J" */
    0x0, 0xa0, 0xa, 0x0, 0xa0, 0xa, 0x0, 0xa0,
    0xa, 0x0, 0xa6, 0xd, 0x2f, 0x80,

    /* U+004B "K" */
    0xa0, 0x34, 0xa0, 0xe0, 0xa2, 0x80, 0xab, 0x0,
    0xbf, 0x80, 0xb0, 0xc0, 0xa0, 0xb0, 0xa0, 0x34,
    0xa0, 0x1c,

    /* U+004C "L" */
    0xa0, 0xa, 0x0, 0xa0, 0xa, 0x0, 0xa0, 0xa,
    0x0, 0xa0, 0xa, 0x0, 0xbf, 0xe0,

    /* U+004D "M" */
    0xb0, 0x7, 0x6d, 0x3, 0xda, 0x80, 0xf6, 0x70,
    0x9d, 0x99, 0x33, 0x64, 0xd8, 0xd9, 0x3d, 0x36,
    0x47, 0xd, 0x90, 0x3, 0x40,

    /* U+004E "N" */
    0xb0, 0xc, 0xb4, 0xc, 0x9c, 0xc, 0x96, 0xc,
    0x93, 0x4c, 0x91, 0xcc, 0x90, 0x9c, 0x90, 0x3c,
    0x90, 0x1c,

    /* U+004F "O" */
    0x7, 0xf8, 0x7, 0x42, 0xc3, 0x40, 0x38, 0xc0,
    0x7, 0x30, 0x1, 0xcc, 0x0, 0x73, 0x40, 0x38,
    0x74, 0x2c, 0x7, 0xf8, 0x0,

    /* U+0050 "P" */
    0xbf, 0xd2, 0x80, 0xda, 0x2, 0xa8, 0xd, 0xbf,
    0xd2, 0x80, 0xa, 0x0, 0x28, 0x0, 0xa0, 0x0,

    /* U+0051 "Q" */
    0x7, 0xf8, 0x1, 0xd0, 0xb0, 0x34, 0x3, 0x83,
    0x0, 0x1c, 0x30, 0x1, 0xc3, 0x0, 0x1c, 0x34,
    0x3, 0x81, 0xd0, 0xb0, 0x7, 0xf8, 0x0, 0x7,
    0x40, 0x0, 0x1f, 0xc0,

    /* U+0052 "R" */
    0xbf, 0xe2, 0x80, 0xda, 0x2, 0xa8, 0xd, 0xbf,
    0xd2, 0x87, 0xa, 0xd, 0x28, 0x1c, 0xa0, 0x38,

    /* U+0053 "S" */
    0xb, 0xe0, 0x28, 0x14, 0x34, 0x0, 0x1d, 0x0,
    0x6, 0xe0, 0x0, 0x2c, 0x0, 0xc, 0x34, 0x2c,
    0x1f, 0xe0,

    /* U+0054 "T" */
    0xbf, 0xfd, 0x2, 0x80, 0x2, 0x80, 0x2, 0x80,
    0x2, 0x80, 0x2, 0x80, 0x2, 0x80, 0x2, 0x80,
    0x2, 0x80,

    /* U+0055 "U" */
    0x90, 0xc, 0x90, 0xc, 0x90, 0xc, 0x90, 0xc,
    0x90, 0xc, 0x90, 0xc, 0xa0, 0xc, 0x34, 0x38,
    0x1f, 0xe0,

    /* U+0056 "V" */
    0xd0, 0xd, 0x70, 0xc, 0x30, 0x28, 0x34, 0x34,
    0x18, 0x30, 0xc, 0x60, 0xd, 0xd0, 0x6, 0xc0,
    0x3, 0x80,

    /* U+0057 "W" */
    0xa0, 0x34, 0xc, 0x70, 0x38, 0x18, 0x30, 0x6c,
    0x28, 0x34, 0xcc, 0x34, 0x28, 0xca, 0x30, 0x19,
    0x87, 0x30, 0xe, 0x43, 0x60, 0xf, 0x3, 0xd0,
    0xf, 0x1, 0xc0,

    /* U+0058 "X" */
    0x70, 0x1c, 0x28, 0x30, 0xc, 0xa0, 0xb, 0xc0,
    0x3, 0x80, 0xa, 0xc0, 0x1c, 0xa0, 0x34, 0x34,
    0x70, 0x1c,

    /* U+0059 "Y" */
    0x38, 0xa, 0xc, 0xc, 0x9, 0x28, 0x3, 0x30,
    0x2, 0xe0, 0x0, 0xc0, 0x0, 0xc0, 0x0, 0xc0,
    0x0, 0xc0,

    /* U+005A "Z" */
    0x3f, 0xfc, 0x0, 0x28, 0x0, 0x70, 0x0, 0xc0,
    0x2, 0x80, 0x7, 0x0, 0xc, 0x0, 0x34, 0x0,
    0x7f, 0xfc,

    /* U+005B "[" */
    0xba, 0x49, 0x24, 0x92, 0x49, 0x24, 0x92, 0x49,
    0x2a,

    /* U+005C "\\" */
    0x80, 0x18, 0x3, 0x0, 0x80, 0x18, 0x3, 0x0,
    0x80, 0x14, 0x3, 0x0, 0x80, 0x14, 0x3,

    /* U+005D "]" */
    0x6c, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc,
    0xc, 0xc, 0xc, 0x6c,

    /* U+005E "^" */
    0x1, 0x0, 0x1e, 0x0, 0x9c, 0x3, 0x24, 0x24,
    0x60, 0xc0, 0xc0,

    /* U+005F "_" */
    0xbf, 0xfc,

    /* U+0060 "`" */
    0x20, 0x1c, 0x5,

    /* U+0061 "a" */
    0x1f, 0xd0, 0x41, 0xc0, 0x7, 0x47, 0x9d, 0x30,
    0x34, 0xc1, 0xd2, 0xf7, 0x40,

    /* U+0062 "b" */
    0xd0, 0x3, 0x40, 0xd, 0x0, 0x3b, 0xf0, 0xf0,
    0x73, 0x40, 0xdd, 0x3, 0x74, 0xc, 0xe0, 0xb3,
    0x7e, 0x0,

    /* U+0063 "c" */
    0xb, 0xe0, 0xe0, 0x43, 0x0, 0x1c, 0x0, 0x30,
    0x0, 0xe0, 0x40, 0xbe, 0x0,

    /* U+0064 "d" */
    0x0, 0x1c, 0x0, 0x70, 0x1, 0xc2, 0xfb, 0x38,
    0x2c, 0xc0, 0x77, 0x1, 0xcc, 0x7, 0x38, 0x2c,
    0x3f, 0x70,

    /* U+0065 "e" */
    0xb, 0xd0, 0xe0, 0xd3, 0x1, 0x9f, 0xfe, 0x30,
    0x0, 0xe0, 0x0, 0xbe, 0x0,

    /* U+0066 "f" */
    0xf, 0x4a, 0x2, 0x82, 0xfc, 0x28, 0xa, 0x2,
    0x80, 0xa0, 0x28, 0xa, 0x0,

    /* U+0067 "g" */
    0xf, 0xfc, 0x34, 0x70, 0x30, 0x30, 0x34, 0x70,
    0x1f, 0x80, 0x30, 0x0, 0x2f, 0xf8, 0x30, 0xc,
    0x70, 0x1c, 0x2f, 0xe0,

    /* U+0068 "h" */
    0xd0, 0xd, 0x0, 0xd0, 0xd, 0xbc, 0xf0, 0xad,
    0x7, 0xd0, 0x7d, 0x7, 0xd0, 0x7d, 0x7,

    /* U+0069 "i" */
    0xd4, 0xd, 0xdd, 0xdd, 0xdd,

    /* U+006A "j" */
    0xd, 0x4, 0x0, 0xd, 0xd, 0xd, 0xd, 0xd,
    0xd, 0xd, 0xd, 0xd, 0x78,

    /* U+006B "k" */
    0xd0, 0x3, 0x40, 0xd, 0x0, 0x34, 0x34, 0xd3,
    0x43, 0x68, 0xf, 0xf0, 0x3c, 0xd0, 0xd0, 0xc3,
    0x42, 0x80,

    /* U+006C "l" */
    0xd3, 0x4d, 0x34, 0xd3, 0x4d, 0x34, 0xd2, 0xc0,

    /* U+006D "m" */
    0xdb, 0xcb, 0xcf, 0xf, 0x7, 0xd0, 0xa0, 0x7d,
    0xa, 0x7, 0xd0, 0xa0, 0x7d, 0xa, 0x7, 0xd0,
    0xa0, 0x70,

    /* U+006E "n" */
    0xdb, 0xcf, 0xa, 0xd0, 0x7d, 0x7, 0xd0, 0x7d,
    0x7, 0xd0, 0x70,

    /* U+006F "o" */
    0xb, 0xe0, 0x38, 0x28, 0x30, 0xc, 0x70, 0xc,
    0x30, 0xc, 0x38, 0x28, 0xb, 0xe0,

    /* U+0070 "p" */
    0xeb, 0xd3, 0x81, 0xcd, 0x3, 0x74, 0xd, 0xd0,
    0x33, 0x82, 0xce, 0xf8, 0x34, 0x0, 0xd0, 0x3,
    0x40, 0x0,

    /* U+0071 "q" */
    0xb, 0xec, 0xe0, 0xb3, 0x1, 0xdc, 0x7, 0x30,
    0x1c, 0xe0, 0xb0, 0xfe, 0xc0, 0x7, 0x0, 0x1c,
    0x0, 0x70,

    /* U+0072 "r" */
    0x0, 0x37, 0xcf, 0x3, 0x40, 0xd0, 0x34, 0xd,
    0x3, 0x40,

    /* U+0073 "s" */
    0x1f, 0x83, 0x0, 0x34, 0x0, 0xb8, 0x0, 0xd1,
    0xa, 0x2f, 0x80,

    /* U+0074 "t" */
    0x18, 0xa, 0xb, 0xf4, 0xa0, 0x28, 0xa, 0x2,
    0x80, 0x70, 0xf, 0x80,

    /* U+0075 "u" */
    0xd0, 0x6d, 0x6, 0xd0, 0x6d, 0x6, 0xd0, 0x6e,
    0x1e, 0x3e, 0x60,

    /* U+0076 "v" */
    0xa0, 0x25, 0xc0, 0xc3, 0x6, 0xa, 0x24, 0xc,
    0xc0, 0x3a, 0x0, 0x74, 0x0,

    /* U+0077 "w" */
    0xa0, 0x70, 0x30, 0xc2, 0xc1, 0x83, 0xd, 0x89,
    0x9, 0x63, 0x30, 0x1a, 0x4c, 0xc0, 0x3c, 0x2a,
    0x0, 0xf0, 0x74, 0x0,

    /* U+0078 "x" */
    0x70, 0x70, 0xa3, 0x0, 0xe8, 0x2, 0xc0, 0xe,
    0x80, 0xd3, 0x46, 0x7, 0x0,

    /* U+0079 "y" */
    0xa0, 0x24, 0xc0, 0xc3, 0x46, 0x6, 0x24, 0xc,
    0xc0, 0x2a, 0x0, 0x34, 0x0, 0xc0, 0xa, 0x1,
    0xe0, 0x0,

    /* U+007A "z" */
    0x3f, 0xe0, 0xc, 0x3, 0x40, 0xa0, 0x1c, 0x3,
    0x40, 0xbf, 0xe0,

    /* U+007B "{" */
    0xe, 0x18, 0x18, 0x18, 0x18, 0x18, 0x70, 0x18,
    0x18, 0x18, 0x18, 0xa,

    /* U+007C "|" */
    0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88,

    /* U+007D "}" */
    0x74, 0x1c, 0xc, 0x8, 0x18, 0xc, 0xf, 0xc,
    0x18, 0xc, 0x1c, 0x64,

    /* U+007E "~" */
    0x2e, 0x4, 0x47, 0xc0,

    /* U+2103 "℃" */
    0x0, 0x0, 0x0, 0xa, 0x40, 0x7e, 0x8, 0x30,
    0xb5, 0xb1, 0x4c, 0x70, 0x0, 0x28, 0x28, 0x0,
    0x0, 0xd, 0x0, 0x0, 0x3, 0x40, 0x0, 0x0,
    0xa0, 0x0, 0x0, 0x1c, 0x0, 0x0, 0x3, 0x80,
    0x80, 0x0, 0x2f, 0xd0, 0x0, 0x0, 0x0,

    /* U+FF05 "％" */
    0x2d, 0x1, 0x42, 0xc, 0x8, 0xc, 0x20, 0x90,
    0x30, 0x82, 0x0, 0xc3, 0x21, 0xb4, 0xf4, 0x8c,
    0x30, 0x8, 0x20, 0xc0, 0x20, 0x83, 0x2, 0x3,
    0xc, 0x14, 0x7, 0xc0,

    /* U+FF0C "，" */
    0x0, 0x2c, 0x2c, 0xc, 0x24, 0x0
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 47, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 0, .adv_w = 67, .box_w = 2, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 5, .adv_w = 99, .box_w = 4, .box_h = 4, .ofs_x = 1, .ofs_y = 6},
    {.bitmap_index = 9, .adv_w = 115, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 25, .adv_w = 115, .box_w = 7, .box_h = 13, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 48, .adv_w = 192, .box_w = 12, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 75, .adv_w = 141, .box_w = 9, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 96, .adv_w = 58, .box_w = 2, .box_h = 4, .ofs_x = 1, .ofs_y = 6},
    {.bitmap_index = 98, .adv_w = 70, .box_w = 3, .box_h = 14, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 109, .adv_w = 70, .box_w = 4, .box_h = 14, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 123, .adv_w = 97, .box_w = 6, .box_h = 5, .ofs_x = 0, .ofs_y = 5},
    {.bitmap_index = 131, .adv_w = 115, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 144, .adv_w = 58, .box_w = 3, .box_h = 5, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 148, .adv_w = 72, .box_w = 4, .box_h = 1, .ofs_x = 0, .ofs_y = 3},
    {.bitmap_index = 149, .adv_w = 58, .box_w = 3, .box_h = 2, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 151, .adv_w = 82, .box_w = 5, .box_h = 12, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 166, .adv_w = 115, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 182, .adv_w = 115, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 196, .adv_w = 115, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 212, .adv_w = 115, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 228, .adv_w = 115, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 244, .adv_w = 115, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 260, .adv_w = 115, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 276, .adv_w = 115, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 292, .adv_w = 115, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 308, .adv_w = 115, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 324, .adv_w = 58, .box_w = 3, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 330, .adv_w = 58, .box_w = 3, .box_h = 10, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 338, .adv_w = 115, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 351, .adv_w = 115, .box_w = 7, .box_h = 4, .ofs_x = 0, .ofs_y = 3},
    {.bitmap_index = 358, .adv_w = 115, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 371, .adv_w = 99, .box_w = 6, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 385, .adv_w = 197, .box_w = 12, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 418, .adv_w = 126, .box_w = 8, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 436, .adv_w = 137, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 452, .adv_w = 133, .box_w = 8, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 470, .adv_w = 143, .box_w = 8, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 488, .adv_w = 123, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 502, .adv_w = 115, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 516, .adv_w = 143, .box_w = 8, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 534, .adv_w = 151, .box_w = 8, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 552, .adv_w = 61, .box_w = 2, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 557, .adv_w = 111, .box_w = 6, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 571, .adv_w = 134, .box_w = 8, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 589, .adv_w = 113, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 603, .adv_w = 169, .box_w = 9, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 624, .adv_w = 150, .box_w = 8, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 642, .adv_w = 154, .box_w = 9, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 663, .adv_w = 132, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 679, .adv_w = 154, .box_w = 10, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 707, .adv_w = 132, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 723, .adv_w = 124, .box_w = 8, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 741, .adv_w = 125, .box_w = 8, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 759, .adv_w = 150, .box_w = 8, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 777, .adv_w = 120, .box_w = 8, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 795, .adv_w = 183, .box_w = 12, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 822, .adv_w = 119, .box_w = 8, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 840, .adv_w = 110, .box_w = 8, .box_h = 9, .ofs_x = -1, .ofs_y = 0},
    {.bitmap_index = 858, .adv_w = 125, .box_w = 8, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 876, .adv_w = 70, .box_w = 3, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 885, .adv_w = 82, .box_w = 5, .box_h = 12, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 900, .adv_w = 70, .box_w = 4, .box_h = 12, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 912, .adv_w = 115, .box_w = 7, .box_h = 6, .ofs_x = 0, .ofs_y = 4},
    {.bitmap_index = 923, .adv_w = 116, .box_w = 8, .box_h = 1, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 925, .adv_w = 126, .box_w = 4, .box_h = 3, .ofs_x = 1, .ofs_y = 8},
    {.bitmap_index = 928, .adv_w = 117, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 941, .adv_w = 129, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 959, .adv_w = 106, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 972, .adv_w = 129, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 990, .adv_w = 115, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1003, .adv_w = 68, .box_w = 5, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1016, .adv_w = 117, .box_w = 8, .box_h = 10, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 1036, .adv_w = 126, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1051, .adv_w = 57, .box_w = 2, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1056, .adv_w = 57, .box_w = 4, .box_h = 13, .ofs_x = -1, .ofs_y = -3},
    {.bitmap_index = 1069, .adv_w = 115, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1087, .adv_w = 59, .box_w = 3, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1095, .adv_w = 193, .box_w = 10, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1113, .adv_w = 127, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1124, .adv_w = 126, .box_w = 8, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1138, .adv_w = 129, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1156, .adv_w = 129, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 1174, .adv_w = 81, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1184, .adv_w = 97, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1195, .adv_w = 78, .box_w = 5, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1207, .adv_w = 126, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1218, .adv_w = 108, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1231, .adv_w = 167, .box_w = 11, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1251, .adv_w = 104, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1264, .adv_w = 108, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 1282, .adv_w = 99, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1293, .adv_w = 70, .box_w = 4, .box_h = 12, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 1305, .adv_w = 56, .box_w = 2, .box_h = 14, .ofs_x = 1, .ofs_y = -4},
    {.bitmap_index = 1312, .adv_w = 70, .box_w = 4, .box_h = 12, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 1324, .adv_w = 115, .box_w = 7, .box_h = 2, .ofs_x = 0, .ofs_y = 4},
    {.bitmap_index = 1328, .adv_w = 208, .box_w = 13, .box_h = 12, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 1367, .adv_w = 208, .box_w = 11, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1395, .adv_w = 208, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = -2}
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
    0, 0, 0, 0, -27, 0, -27, 0,
    0, 0, 0, -13, 0, -22, -3, 0,
    0, 0, 0, -3, 0, 0, 0, 0,
    -8, 0, 0, 0, 0, 0, -5, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -5, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 18, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -22, 0, -32,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -24, -5, -16, -8, 0,
    -22, 0, 0, 0, -3, 0, 0, 0,
    5, 0, 0, -11, 0, -8, -6, 0,
    -5, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, -5,
    -5, -11, 0, -5, -3, -7, -16, -5,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -7, 0, -2, 0, -4, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -10, -3, -19, 0, 0,
    0, 0, 0, 0, 0, 0, 0, -6,
    -8, 0, -3, 5, 5, 0, 0, 1,
    -5, 0, 0, 0, 0, 0, 0, 0,
    0, -12, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, -7, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -13, 0, -22,
    0, 0, 0, 0, 0, 0, -7, -2,
    -3, 0, 0, -13, -4, -4, 0, 0,
    -4, -2, -10, 5, 0, -3, 0, 0,
    0, 0, 5, -4, -2, -2, -1, -1,
    -2, 0, 0, 0, 0, -8, 0, 0,
    0, 0, 0, 0, 0, 0, 0, -4,
    -4, -6, 0, -2, -1, -1, -4, -1,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -3, 0, -4, -3, -3, -4, 0,
    0, 0, 0, 0, 0, -7, 0, 0,
    0, 0, 0, 0, -7, -3, -6, -5,
    -4, -1, -1, -1, -2, -3, 0, 0,
    0, 0, -5, 0, 0, 0, 0, -7,
    -3, -4, -3, 0, -4, 0, 0, 0,
    0, -9, 0, 0, 0, -5, 0, 0,
    0, -3, 0, -10, 0, -6, 0, -3,
    -2, -5, -5, -5, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -4, 0, -2, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, -3, 0, 0, 0,
    0, 0, 0, -6, 0, -3, 0, -8,
    -3, 0, 0, 0, 0, 0, -17, 0,
    -17, -17, 0, 0, 0, -9, -3, -33,
    -5, 0, 0, 0, 0, -6, 0, -8,
    0, -8, -4, 0, -6, 0, 0, -5,
    -5, -3, -4, -5, -4, -7, -4, -7,
    0, 0, 0, -7, 0, 0, 0, 0,
    0, 0, 0, -1, 0, 0, 0, -5,
    0, -4, -1, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, -6, 0, -6, 0, 0, 0,
    0, 0, 0, -10, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -5, 0, -10,
    0, -7, 0, 0, 0, 0, -2, -3,
    -5, 0, -2, -5, -4, -3, -3, 0,
    -4, 0, 0, 0, -2, 0, 0, 0,
    -3, 0, 0, -8, -4, -5, -4, -4,
    -5, -4, 0, -21, 0, -36, 0, -13,
    0, 0, 0, 0, -8, 0, -7, 0,
    -6, -28, -7, -18, -14, 0, -18, 0,
    -19, 0, -3, -4, -1, 0, 0, 0,
    0, -5, -3, -9, -8, 0, -9, 0,
    0, 0, 0, 0, -27, -8, -27, -19,
    0, 0, 0, -12, 0, -35, -3, -6,
    0, 0, 0, -6, -3, -20, 0, -11,
    -6, 0, -8, 0, 0, 0, -3, 0,
    0, 0, 0, -4, 0, -5, 0, 0,
    0, -3, 0, -7, 0, 0, 0, 0,
    0, -1, 0, -5, -4, -4, 0, 1,
    1, -1, -1, -3, 0, -1, -3, 0,
    -1, 0, 0, 0, 0, 0, 0, 0,
    0, -2, 0, -2, 0, 0, 0, -4,
    0, 3, 0, 0, 0, 0, 0, 0,
    0, -4, -4, -5, 0, 0, 0, 0,
    -4, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -6, 0, 0, 0, 0,
    0, -1, 0, 0, 0, 0, -25, -17,
    -25, -22, -5, -5, 0, -10, -6, -30,
    -10, 0, 0, 0, 0, -5, -4, -13,
    0, -17, -16, -5, -17, 0, 0, -11,
    -14, -5, -11, -8, -9, -10, -8, -18,
    0, 0, 0, 0, -4, 0, -4, -8,
    0, 0, 0, -4, 0, -11, -3, 0,
    0, -1, 0, -3, -4, 0, 0, -1,
    0, 0, -3, 0, 0, 0, -1, 0,
    0, 0, 0, -2, 0, 0, 0, 0,
    0, 0, -16, -5, -16, -12, 0, 0,
    0, -4, -3, -17, -3, 0, -3, 2,
    0, 0, 0, -5, 0, -6, -4, 0,
    -6, 0, 0, -5, -3, 0, -8, -3,
    -3, -4, -3, -6, 0, 0, 0, 0,
    -8, -3, -8, -8, 0, 0, 0, 0,
    -2, -16, -2, 0, 0, 0, 0, 0,
    0, -2, 0, -4, 0, 0, -4, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -3, 0, -3, 0, -3, 0, -7,
    0, 0, 0, 0, 0, 0, -5, -1,
    -4, -5, -3, 0, 0, 0, 0, 0,
    0, -3, -2, -4, 0, 0, 0, 0,
    0, -4, -3, -4, -4, -3, -4, -4,
    0, 0, 0, 0, -21, -16, -21, -16,
    -6, -6, -2, -4, -4, -24, -4, -4,
    -3, 0, 0, 0, 0, -7, 0, -16,
    -10, 0, -15, 0, 0, -10, -10, -7,
    -8, -4, -6, -8, -4, -11, 0, 0,
    0, 0, 0, -9, 0, 0, 0, 0,
    0, -2, -5, -8, -8, 0, -3, -2,
    -2, 0, -4, -4, 0, -4, -6, -5,
    -4, 0, 0, 0, 0, -4, -6, -4,
    -4, -6, -4, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -21, -8, -13, -8, 0,
    -17, 0, 0, 0, 0, 0, 7, 0,
    16, 0, 0, 0, 0, -5, -3, 0,
    2, 0, 0, 0, 0, -13, 0, 0,
    0, 0, 0, 0, -3, 0, 0, 0,
    0, -6, 0, -4, -1, 0, -6, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -4, 0, 0, 0, 0, 0, 0,
    0, -8, 0, -7, -3, 1, -3, 0,
    0, 0, -3, 0, 0, 0, 0, -14,
    0, -5, 0, -1, -11, 0, -7, -4,
    0, -1, 0, 0, 0, 0, -1, -5,
    0, -1, -1, -5, -1, -2, 0, 0,
    0, 0, 0, -5, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -5, 0, -4,
    0, 0, -6, 0, 0, -3, -6, 0,
    -3, 0, 0, 0, 0, -3, 0, 1,
    1, 1, 1, 0, 0, 0, 0, -9,
    0, 2, 0, 0, 0, 0, -2, 0,
    0, -5, -5, -6, 0, -4, -3, 0,
    -7, 0, -5, -4, 0, -1, -3, 0,
    0, 0, 0, -3, 0, 1, 1, -2,
    1, 0, 3, 9, 11, 0, -12, -4,
    -12, -4, 0, 0, 6, 0, 0, 0,
    0, 10, 0, 15, 10, 7, 13, 0,
    14, -5, -3, 0, -4, 0, -3, 0,
    -1, 0, 0, 2, 0, -1, 0, -4,
    0, 0, 3, -8, 0, 0, 0, 11,
    0, 0, -9, 0, 0, 0, 0, -7,
    0, 0, 0, 0, -4, 0, 0, -4,
    -4, 0, 0, 0, 8, 0, 0, 0,
    0, -1, -1, 0, 3, -4, 0, 0,
    0, -9, 0, 0, 0, 0, 0, 0,
    -2, 0, 0, 0, 0, -6, 0, -3,
    0, 0, -4, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, -6,
    2, -10, 2, 0, 2, 2, -3, 0,
    0, 0, 0, -9, 0, 0, 0, 0,
    -3, 0, 0, -3, -5, 0, -3, 0,
    -3, 0, 0, -5, -4, 0, 0, -2,
    0, -2, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 1, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -6, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, -5,
    0, -4, 0, 0, -8, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, -13, -6, -13, -9, 5, 5,
    0, -4, 0, -13, 0, 0, 0, 0,
    0, 0, 0, -3, 2, -6, -3, 0,
    -3, 0, 0, 0, -1, 0, 0, 5,
    4, 0, 5, -1, 0, 0, 0, -12,
    0, 2, 0, 0, 0, 0, -3, 0,
    0, 0, 0, -6, 0, -3, 0, 0,
    -5, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -5, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 1, -7,
    1, 2, 3, 3, -7, 0, 0, 0,
    0, -4, 0, 0, 0, 0, -1, 0,
    0, -6, -4, 0, -3, 0, 0, 0,
    -3, -5, 0, 0, 0, -4, 0, 0,
    0, 0, 0, -3, -8, -2, -8, -5,
    0, 0, 0, -3, 0, -10, 0, -5,
    0, -3, 0, 0, -4, -3, 0, -5,
    -1, 0, 0, 0, -3, 0, 0, 0,
    0, 0, 0, 0, 0, -6, 0, 0,
    0, -3, -9, 0, -9, -2, 0, 0,
    0, -1, 0, -8, 0, -6, 0, -3,
    0, -4, -6, 0, 0, -3, -1, 0,
    0, 0, -3, 0, 0, 0, 0, 0,
    0, 0, 0, -5, -4, 0, 0, -6,
    1, -4, -2, 0, 0, 1, 0, 0,
    -3, 0, -1, -8, 0, -4, 0, -3,
    -8, 0, 0, -3, -5, 0, 0, 0,
    0, 0, 0, -6, 0, 0, 0, 0,
    -1, 0, 0, 0, 0, 0, -8, 0,
    -8, -4, 0, 0, 0, 0, 0, -10,
    0, -5, 0, -1, 0, -1, -2, 0,
    0, -5, -1, 0, 0, 0, -3, 0,
    0, 0, 0, 0, 0, -4, 0, -6,
    0, 0, 0, 0, 0, -4, 0, 0,
    0, 0, 0, 0, 0, 0, 0, -7,
    0, 0, 0, 0, -8, 0, 0, -6,
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
const lv_font_t my_font_source_han_13 = {
#else
lv_font_t my_font_source_han_13 = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 15,          /*The maximum line height required by the font*/
    .base_line = 4,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = -2,
    .underline_thickness = 1,
#endif
    .static_bitmap = 0,
    .dsc = &font_dsc,          /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
#if LV_VERSION_CHECK(8, 2, 0) || LVGL_VERSION_MAJOR >= 9
    .fallback = NULL,
#endif
    .user_data = NULL,
};



#endif /*#if MY_FONT_SOURCE_HAN_13*/
