#ifndef __FONTS_H_
#define __FONTS_H_

#include <stdint.h>

/* Partially based on original code for the KS0108 by Stephane Rey */
/* Based on code code by Kevin Townsend */

typedef struct {
    const uint8_t widthBits; // width, in bits (or pixels), of the character
} FONT_CHAR_INFO;

struct FONT_DEF {
    uint8_t u8Width;                /* Character width for storage          */
    uint8_t u8Height;               /* Character height for storage         */
    uint8_t u8FirstChar;            /* The first character available        */
    uint8_t u8LastChar;             /* The last character available         */
    const uint8_t *au8FontTable;    /* Font table start address in memory   */
    const FONT_CHAR_INFO *charInfo; /* Pointer to array of char information */
    const uint16_t *charExtra;      /* Pointer to array of extra char info  */
};

struct EXTFONT {
    char type; // 0: none, 1: static, 2: loaded
    char name[13];
    struct FONT_DEF def;
};

typedef const struct FONT_DEF * FONT;

#define FONT_DEFAULT 0
#define FONT_INTERNAL 1
#define FONT_EXTERNAL 2

extern const struct FONT_DEF * font;
extern struct EXTFONT efont;

#endif
