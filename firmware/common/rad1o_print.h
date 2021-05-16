#ifndef _PRINT_H
#define _PRINT_H 1

#include <stdint.h>
void lcdPrint(const char *string);
void lcdNl(void);
void lcdCheckNl(void);
void lcdPrintln(const char *string);
void lcdPrintInt(int number);
void lcdClear();
void lcdRefresh();
void lcdMoveCrsr(signed int dx,signed int dy);
void lcdSetCrsr(int dx,int dy);
void lcdSetCrsrX(int dx);
int lcdGetCrsrX();
int lcdGetCrsrY();
void setSystemFont(void);
int lcdGetVisibleLines(void);

#endif /* _PRINT_H */
