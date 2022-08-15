#include "print.h"
#include "display.h"
#include "fonts.h"
#include "render.h"
#include "smallfonts.h"

static int32_t x = 0;
static int32_t y = 0;

void rad1o_lcdPrint(const char* string)
{
	x = rad1o_DoString(x, y, string);
}

void rad1o_lcdNl(void)
{
	x = 0;
	y += rad1o_getFontHeight();
}

void rad1o_lcdClear(void)
{
	x = 0;
	y = 0;
	rad1o_lcdFill(0xff);
}

void rad1o_lcdMoveCrsr(int32_t dx, int32_t dy)
{
	x += dx;
	y += dy;
}

void rad1o_lcdSetCrsr(int32_t dx, int32_t dy)
{
	x = dx;
	y = dy;
}

void rad1o_setSystemFont(void)
{
	rad1o_setIntFont(&Font_7x8);
}
