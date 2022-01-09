#include "render.h"
#include "decoder.h"
#include "display.h"
#include "fonts.h"
#include "smallfonts.h"

#include <string.h>

/* Global Variables */
static const struct FONT_DEF* font = NULL;

static struct EXTFONT efont;

static uint8_t color_bg = 0xff; /* background color */
static uint8_t color_fg = 0x00; /* foreground color */

/* Exported Functions */
void rad1o_setTextColor(uint8_t bg, uint8_t fg)
{
    color_bg = bg;
    color_fg = fg;
}

void rad1o_setIntFont(const struct FONT_DEF* newfont)
{
    memcpy(&efont.def, newfont, sizeof(struct FONT_DEF));
    efont.type = FONT_INTERNAL;
    font = &efont.def;
}

int rad1o_getFontHeight(void)
{
    if (font)
        return font->u8Height;
    return 8; // XXX: Should be done right.
}

static int
_getIndex(int c)
{
#define ERRCHR (font->u8FirstChar + 1)
    /* Does this font provide this character? */
    if (c < font->u8FirstChar)
        c = ERRCHR;
    if (c > font->u8LastChar && efont.type != FONT_EXTERNAL && font->charExtra == NULL)
        c = ERRCHR;

    if (c > font->u8LastChar && (efont.type == FONT_EXTERNAL || font->charExtra != NULL)) {
        int cc = 0;
        while (font->charExtra[cc] < c)
            cc++;
        if (font->charExtra[cc] > c)
            c = ERRCHR;
        else
            c = font->u8LastChar + cc + 1;
    };
    c -= font->u8FirstChar;
    return c;
}

int rad1o_DoChar(int sx, int sy, int c)
{
    if (font == NULL) {
        font = &Font_7x8;
    };

    /* how many bytes is it high? */
    char height = (font->u8Height - 1) / 8 + 1;
    char hoff = (8 - (font->u8Height % 8)) % 8;

    const uint8_t* data;
    int width, preblank = 0, postblank = 0;
    do { /* Get Character data */
        /* Get intex into character list */
        c = _getIndex(c);

        /* starting offset into character source data */
        int toff = 0;

        if (font->u8Width == 0) {
            for (int y = 0; y < c; y++)
                toff += font->charInfo[y].widthBits;
            width = font->charInfo[c].widthBits;

            toff *= height;
            data = &font->au8FontTable[toff];
            postblank = 1;
        } else if (font->u8Width == 1) { // NEW CODE
            // Find offset and length for our character
            for (int y = 0; y < c; y++)
                toff += font->charInfo[y].widthBits;
            width = font->charInfo[c].widthBits;
            if (font->au8FontTable[toff] >> 4 == 15) { // It's a raw character!
                preblank = font->au8FontTable[toff + 1];
                postblank = font->au8FontTable[toff + 2];
                data = &font->au8FontTable[toff + 3];
                width = (width - 3 / height);
            } else {
                data = rad1o_pk_decode(&font->au8FontTable[toff], &width);
            }
        } else {
            toff = (c)*font->u8Width * 1;
            width = font->u8Width;
            data = &font->au8FontTable[toff];
        };

    } while (0);

#define xy_(x, y) \
    ((x < 0 || y < 0 || x >= RESX || y >= RESY) ? 0 : (y)*RESX + (x))
#define gPx(x, y) (data[x * height + (height - y / 8 - 1)] & (1 << (y % 8)))

    int x = 0;

    /* Fonts may not be byte-aligned, shift up so the top matches */
    sy -= hoff;

    sx += preblank;

    uint8_t* lcdBuffer = rad1o_lcdGetBuffer();
    /* per line */
    for (int y = hoff; y < height * 8; y++) {
        if (sy + y >= RESY)
            continue;

        /* Optional: empty space to the left */
        for (int b = 1; b <= preblank; b++) {
            if (sx >= RESX)
                continue;
            lcdBuffer[xy_(sx - b, sy + y)] = color_bg;
        };
        /* Render character */
        for (x = 0; x < width; x++) {
            if (sx + x >= RESX)
                continue;
            if (gPx(x, y)) {
                lcdBuffer[xy_(sx + x, sy + y)] = color_fg;
            } else {
                lcdBuffer[xy_(sx + x, sy + y)] = color_bg;
            };
        };
        /* Optional: empty space to the right */
        for (int m = 0; m < postblank; m++) {
            if (sx + x + m >= RESX)
                continue;
            lcdBuffer[xy_(sx + x + m, sy + y)] = color_bg;
        };
    };
    return sx + (width + postblank);
}

int rad1o_DoString(int sx, int sy, const char* s)
{
    const char* c;
    for (c = s; *c != 0; c++) {
        sx = rad1o_DoChar(sx, sy, *c);
    };
    return sx;
}
