#include "display.h"

#include "gpio_lpc.h"
#include "hackrf_core.h"
#include "delay.h"

#include <libopencm3/lpc43xx/scu.h>
#include <libopencm3/lpc43xx/ssp.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static void delayms(const uint32_t milliseconds)
{
	/* NOTE: Naively assumes 204 MHz instruction cycle clock and five instructions per count */
	delay(milliseconds * 40800);
}

static struct gpio gpio_lcd_cs = GPIO(4, 12);    /* P9_0 */
static struct gpio gpio_lcd_bl_en = GPIO(0, 8);  /* P1_1 */
static struct gpio gpio_lcd_reset = GPIO(5, 17); /* P9_4 */

/**************************************************************************/
/* Utility routines to manage nokia display */
/**************************************************************************/

static void rotate(void);

static uint8_t lcdBuffer[RESX * RESY];

static bool isTurned;

static void select(void)
{
	/*
     * The LCD requires 9-Bit frames
     * Freq = PCLK / (CPSDVSR * [SCR+1])
     * We want 120ns / bit -> 8.3 MHz.
     * SPI1 BASE CLOCK is expected to be 204 MHz.
     * 204 MHz / ( 12 * (1 + 1)) = 8.5 MHz
     *
     * Set CPSDVSR = 12
     */
	uint8_t serial_clock_rate = 1;
	uint8_t clock_prescale_rate = 12;

	ssp_init(
		LCD_SSP,
		SSP_DATA_9BITS,
		SSP_FRAME_SPI,
		SSP_CPOL_0_CPHA_0,
		serial_clock_rate,
		clock_prescale_rate,
		SSP_MODE_NORMAL,
		SSP_MASTER,
		SSP_SLAVE_OUT_ENABLE);

	gpio_clear(&gpio_lcd_cs);
}

static void deselect(void)
{
	gpio_set(&gpio_lcd_cs);
}

static void write(uint8_t cd, uint8_t data)
{
	uint16_t frame = 0x0;

	frame = cd << 8;
	frame |= data;

	ssp_transfer(LCD_SSP, frame);
}

void rad1o_lcdInit(void)
{
	gpio_output(&gpio_lcd_bl_en);
	gpio_output(&gpio_lcd_reset);
	gpio_output(&gpio_lcd_cs);

	/* prepare SPI */
	SETUPpin(LCD_DI);
	SETUPpin(LCD_SCK);

	// Reset the display
	gpio_clear(&gpio_lcd_reset);
	delayms(100);
	gpio_set(&gpio_lcd_reset);
	delayms(100);

	select();

	/* The controller is a PCF8833 - documentation can be found online. */
	static uint8_t initseq_d[] = {
		0x11, // SLEEP_OUT  (wake up)
		0x3A,
		2, // mode 8bpp  (2= 8bpp, 3= 12bpp, 5= 16bpp)
		0x36,
		0b11000000, // my,mx,v,lao,rgb,x,x,x
		0x25,
		0x3a, // set contrast
		0x29, // display on
		0x03, // BSTRON (booster voltage)
		0x2A,
		1,
		RESX,
		0x2B,
		1,
		RESY};
	uint16_t initseq_c =
		~(/* commands: 1, data: 0 */
		  (1 << 0) | (1 << 1) | (0 << 2) | (1 << 3) | (0 << 4) | (1 << 5) |
		  (0 << 6) | (1 << 7) | (1 << 8) | (1 << 9) | (0 << 10) | (0 << 11) |
		  (1 << 12) | (0 << 13) | (0 << 14) | 0);

	write(0, 0x01); /* most color displays need the pause */
	delayms(10);

	size_t i = 0;
	while (i < sizeof(initseq_d)) {
		write(initseq_c & 1, initseq_d[i++]);
		initseq_c = initseq_c >> 1;
	}
	deselect();
	rad1o_lcdFill(0xff); /* Clear display buffer */
	rotate();

	gpio_set(&gpio_lcd_bl_en);
}

void rad1o_lcdDeInit(void)
{
	gpio_clear(&gpio_lcd_bl_en);
	rad1o_lcdFill(0xff);
	rad1o_lcdDisplay();
}

void rad1o_lcdFill(uint8_t f)
{
	memset(lcdBuffer, f, RESX * RESY);
}

void rad1o_lcdSetPixel(uint8_t x, uint8_t y, uint8_t f)
{
	if (x > RESX || y > RESY)
		return;
	lcdBuffer[y * RESX + x] = f;
}

static uint8_t getPixel(uint8_t x, uint8_t y)
{
	return lcdBuffer[y * RESX + x];
}

uint8_t* rad1o_lcdGetBuffer(void)
{
	return lcdBuffer;
}

void rad1o_lcdDisplay(void)
{
	select();

	uint16_t x, y;

	/* set (back) to 8 bpp mode */
	write(TYPE_CMD, 0x3a);
	write(TYPE_DATA, 2);

	write(TYPE_CMD, 0x2C); // memory write (RAMWR)

	for (y = 0; y < RESY; y++) {
		for (x = 0; x < RESX; x++) {
			write(TYPE_DATA, getPixel(x, y));
		};
	}
	deselect();
}

static void rotate(void)
{
	select();
	write(TYPE_CMD, 0x36); // MDAC-Command
	if (isTurned) {
		write(TYPE_DATA, 0b01100000); // my,mx,v,lao,rgb,x,x,x
		isTurned = true;
	} else {
		write(TYPE_DATA, 0b11000000); // my,mx,v,lao,rgb,x,x,x
		isTurned = false;
	}
	deselect();
	rad1o_lcdDisplay();
}
