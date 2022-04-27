#ifndef __RENDER_H_
#define __RENDER_H_

#include "fonts.h"

void rad1o_setTextColor(uint8_t bg, uint8_t fg);
void rad1o_setIntFont(const struct FONT_DEF *font);
int rad1o_getFontHeight(void);
int rad1o_DoString(int sx, int sy, const char *s);
int rad1o_DoChar(int sx, int sy, int c);
#endif
