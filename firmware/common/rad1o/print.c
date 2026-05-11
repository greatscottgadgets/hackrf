#include "print.h"

#include <alloca.h>
#include <stdarg.h>
#include <stddef.h>

#include <picoprintf.h>

#include "display.h"
#include "render.h"
#include "smallfonts.h"

static int32_t x = 0;
static int32_t y = 0;

void rad1o_lcdPrint(const char* fmt, ...)
{
	va_list args1, args2;
	va_start(args1, fmt);
	va_copy(args2, args1);
	int buflen = pico_vsnprintf(NULL, 0, fmt, args1) + 1;
	va_end(args1);
	char* buf = alloca(buflen);
	pico_vsnprintf(buf, buflen, fmt, args2);
	va_end(args2);
	x = rad1o_DoString(x, y, buf);
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
