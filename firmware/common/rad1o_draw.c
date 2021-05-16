#include "rad1o_display.h"

#define MAX_LINE_THICKNESS 16

#define SWAP(p1, p2) do { int SWAP = p1; p1 = p2; p2 = SWAP; } while (0)
#define ABS(p) (((p)<0) ? -(p) : (p))

void drawHLine(int y, int x1, int x2, uint8_t color) {
	if(x1>x2) {
		SWAP(x1, x2);
	}
    for (int i=x1; i<=x2; ++i) {
        lcdSetPixel(i, y, color);
    }
}

void drawVLine(int x, int y1, int y2, uint8_t color) {
	if(y1>y2) {
		SWAP(y1,y2);
	}
    for (int i=y1; i<=y2; ++i) {
        lcdSetPixel(x, i, color);
    }
}

void drawRectFill(int x, int y, int width, int heigth, uint8_t color) {
    for (int i=y; i<y+heigth; ++i) {
        drawHLine(i, x, x+width-1, color);
    }
}

void drawLine(int x1, int y1, int x2, int y2, uint8_t color, int thickness) {
	if(thickness<1) {
		thickness = 1;
	}
	if(thickness>MAX_LINE_THICKNESS) {
		thickness = MAX_LINE_THICKNESS;
	}
	bool xSwap = x1 > x2;
	bool ySwap = y1 > y2;
	if(x1==x2) {
		if(ySwap) { SWAP(y1,y2); }
		drawRectFill(x1-thickness/2, y1, thickness, y2-y1, color);
		return;
	}
	if(y1==y2) {
		if(xSwap) { SWAP(x1,x2); }
		drawRectFill(x1, y1-thickness/2, x2-x1, thickness, color);
		return;
	}
	if(xSwap){
		x1 = -x1;
		x2 = -x2;
	}
	if(ySwap){
		y1 = -y1;
		y2 = -y2;
	}
	bool mSwap = ABS(x2-x1) < ABS(y2-y1);
	if(mSwap) {
		SWAP(x1,y1);
		SWAP(x2,y2);
	}
	int dx = x2-x1;
	int dy = y2-y1;
	int D = 2*dy - dx;
	
	// lcdSetPixel(x1, y1, color);
	int y = y1;
	for(int x = x1; x <= x2; x++) {
		int px = mSwap ? y : x;
		if(xSwap) {
			px = -px;
		}
		int py = mSwap ? x : y;
		if(ySwap) {
			py = -py;
		}
		lcdSetPixel(px, py, color);
		if(D > 0) {
			y++;
			D += 2 * dy - 2 * dx;
		} else {
			D += 2 * dy;
		}
		
		for(int t=1; t<thickness; t++) {
			int offset = ((t-1)/2+1)*(t%2*2-1);
			int tx = px;
			int ty = py;
			if(mSwap) {
				tx += offset;
			} else {
				ty += offset;
			}
			lcdSetPixel(tx, ty, color);
		}
	}
}
