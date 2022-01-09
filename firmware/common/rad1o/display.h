#ifndef __RAD1O_DISPLAY_H__
#define __RAD1O_DISPLAY_H__

#include <libopencm3/cm3/common.h>

#include <stdint.h>

#define RESX 130
#define RESY 130

#define TYPE_CMD 0
#define TYPE_DATA 1

#define _PIN(pin, func, ...) pin
#define _FUNC(pin, func, ...) func
#define SETUPpin(args...) scu_pinmux(_PIN(args), _FUNC(args))

#define LCD_DI P1_4, SCU_CONF_FUNCTION5 | SCU_SSP_IO
#define LCD_SCK P1_19, SCU_CONF_FUNCTION1 | SCU_SSP_IO

#define LCD_SSP SSP1_NUM

void rad1o_lcdInit(void);
void rad1o_lcdDeInit(void);
void rad1o_lcdFill(uint8_t f);
void rad1o_lcdDisplay(void);
void rad1o_lcdSetPixel(uint8_t x, uint8_t y, uint8_t f);
uint8_t* rad1o_lcdGetBuffer(void);

#endif
