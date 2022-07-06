#include "display.h"

#include <stdint.h>

#define SWAP(p1, p2)               \
	do {                       \
		uint8_t SWAP = p1; \
		p1 = p2;           \
		p2 = SWAP;         \
	} while (0)

void rad1o_drawHLine(uint8_t y, uint8_t x1, uint8_t x2, uint8_t color)
{
	if (x1 > x2) {
		SWAP(x1, x2);
	}
	for (uint8_t i = x1; i <= x2; ++i) {
		rad1o_lcdSetPixel(i, y, color);
	}
}

void rad1o_drawVLine(uint8_t x, uint8_t y1, uint8_t y2, uint8_t color)
{
	if (y1 > y2) {
		SWAP(y1, y2);
	}
	for (uint8_t i = y1; i <= y2; ++i) {
		rad1o_lcdSetPixel(x, i, color);
	}
}
