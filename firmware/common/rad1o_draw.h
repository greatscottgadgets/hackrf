#ifndef F1RMWARE_DRAW_H
#define F1RMWARE_DRAW_H

void drawHLine(int y, int x1, int x2, uint8_t color);
void drawVLine(int x, int y1, int y2, uint8_t color);

void drawRectFill(int x, int y, int width, int heigth, uint8_t color);

void drawLine(int x1, int y1, int x2, int y2, uint8_t color, int thickness);

#endif //F1RMWARE_DRAW_H
