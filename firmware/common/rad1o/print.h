#ifndef __RAD1O_PRINT_H__
#define __RAD1O_PRINT_H__

#include <stdint.h>

void rad1o_lcdPrint(const char* string);
void rad1o_lcdNl(void);
void rad1o_lcdClear(void);
void rad1o_lcdMoveCrsr(int32_t dx, int32_t dy);
void rad1o_lcdSetCrsr(int32_t dx, int32_t dy);
void rad1o_setSystemFont(void);

#endif
