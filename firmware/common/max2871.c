#include "mixer.h"
//#include "max2871.h"
//#include "mac2871_regs.def" // private register def macros

#if (defined DEBUG)
#include <stdio.h>
#define LOG printf
#else
#define LOG(x,...)
#include <libopencm3/lpc43xx/ssp.h>
#include <libopencm3/lpc43xx/scu.h>
#include <libopencm3/lpc43xx/gpio.h>
#include "hackrf_core.h"
#endif

#include <stdint.h>
#include <string.h>

static void max2871_spi_write(uint8_t r, uint32_t v);
static void delay_ms(int ms);

static uint32_t registers[6];
/*
 * - The input is fixed to 50 MHz
 * f_REF = 50 MHz
 *
 * - The VCO must operate between 3 GHz and 6 GHz
 * f_VCO range: 3 GHz - 6 GHz
 *
 * Phase Detector Clock:
 *      - We configure the phase detector to not change the
 *        input signal
 *      f_PFD = f_REF x [(1 + DBR)/(R x (1 + RDIV2))]
 *      R = 1
 *      DBR = 0
 *      RDIV2 = 0
 *
 *      => f_PFD = 50 MHz
 *
 *
 * f_RFOUT < 3 GHz:
 *  - To go to < 3 GHz, f_VCO has to be divided.
 *  f_RFOUT = f_VCO / DIVA
 *
 *  - We just divide by 2 for now
 *  DIVA = 2
 *  => f_RFOUT = f_VCO / 2
 *  
 *  - Relationship between f_PFD (fixed) and f_VCO (variable)
 *  N + (F/M) = f_VCO/ f_PFD
 *
 *  - Insert relationship between f_RFOUT and f_VCO:
 *  N + (F/M) = (f_RFOUT * 2) / f_PFD
 *  f_RFOUT = (N + F/M) * f_PFD / 2
 *
 *  - Limits for N, M and F from the datasheet:
 *  19 <= N <= 4091
 *  2  <= M <= 4095
 *  F < M
 *
 *  - Plug in constants:
 *  f_RFOUT = (N + F/M) / 2 * 50 MHz
 *
 *  - Given the range of N, we can go to:
 *  f_RFOUT range: 475 MHz - 3 GHz
 *
 *  - N steps in 25 MHz increments:
 *  N = floor(f_RFOUT / 50 MHz * 2)
 *      => N = (f_RFOUT * 2) / 50 MHz (uses integer math)
 *
 *  - Calculate the error:
 *  f_int_ERROR = f_RFOUT - (N * 50 MHz) / 2
 *  f_int_ERROR range: 0 MHz - 24 MHz
 *
 *  - Use the fraction to get to the correct frequency:
 *  (F/M) / 2 * 50 MHz = f_int_ERROR
 *
 *  - Fix M:
 *  M = 25
 * 
 *  - Calculate F:
 *  (F/25) / 2 * 50 MHz = f_int_ERROR
 *  F = f_int_ERROR / 1 MHz
 *
 *  - Calculate the new error:
 *
 *  f_ERROR = f_RFOUT - (N + F/M) / 2 * 50 MHz
 *  f_ERROR range: 0 MHz - 1 MHz
 *
 *
 * f_RFOUT > 3 GHz:
 *  - Do not divide the VCO output
 *  DIVA = 1
 *  => f_RFOUT = f_VCO
 *
 *  - Relationship between f_PFD (fixed) and f_VCO (variable)
 *  N + (F/M) = f_VCO/ f_PFD
 *
 *  - Insert relationship between f_RFOUT and f_VCO:
 *  N + (F/M) = f_RFOUT / f_PFD
 *  f_RFOUT = (N + F/M) * f_PFD
 *
 *  - Limits for N, M and F from the datasheet:
 *  19 <= N <= 4091
 *  2  <= M <= 4095
 *  F < M
 *
 *  - Plug in constants:
 *  f_RFOUT = (N + F/M) * 50 MHz
 *
 *  - Given the range of N and the VCO limits, we can go to:
 *  f_RFOUT range: 3000 MHz - 6000 MHz
 *
 *  - N steps in 50 MHz increments:
 *  N = floor(f_RFOUT / 50 MHz)
 *      => N = f_RFOUT / 50 MHz (uses integer math)
 *
 *  - Calculate the error:
 *  f_int_ERROR = f_RFOUT - N * 50 MHz
 *  f_int_ERROR range: 0 MHz - 49 MHz
 *
 *  - Use the fraction to get to the correct frequency:
 *  (F/M) * 50 MHz = f_int_ERROR
 *
 *  - Fix M:
 *  M = 50
 * 
 *  - Calculate F:
 *  (F/50) * 50 MHz = f_int_ERROR
 *  F = f_int_ERROR / 1 MHz
 *
 *  - Calculate the new error:
 *
 *  f_ERROR = f_RFOUT - (N + F/M) * 50 MHz
 *  f_ERROR range: 0 MHz - 1 MHz
 *
 */

void mixer_setup(void)
{
	/* Configure GPIO pins. */
	scu_pinmux(SCU_VCO_CE, SCU_GPIO_FAST);
	//scu_pinmux(SCU_VCO_SCLK, SCU_GPIO_FAST | SCU_CONF_FUNCTION4);
	scu_pinmux(SCU_VCO_SCLK, SCU_GPIO_FAST);
	scu_pinmux(SCU_VCO_SDATA, SCU_GPIO_FAST);
	scu_pinmux(SCU_VCO_LE, SCU_GPIO_FAST);

	/* Set GPIO pins as outputs. */
	GPIO_DIR(PORT_VCO_CE) |= PIN_VCO_CE;
	GPIO_DIR(PORT_VCO_SCLK) |= PIN_VCO_SCLK;
	GPIO_DIR(PORT_VCO_SDATA) |= PIN_VCO_SDATA;
	GPIO_DIR(PORT_VCO_LE) |= PIN_VCO_LE;

	/* set to known state */
	gpio_set(PORT_VCO_CE, PIN_VCO_CE); /* active high */
	gpio_clear(PORT_VCO_SCLK, PIN_VCO_SCLK);
	gpio_clear(PORT_VCO_SDATA, PIN_VCO_SDATA);
	gpio_set(PORT_VCO_LE, PIN_VCO_LE); /* active low */

    registers[0] = 0x007D0000;
    registers[1] = 0x2000FFF9;
    registers[2] = 0x00004042;
    registers[3] = 0x0000000B;
    registers[4] = 0x6180B23C;
    registers[5] = 0x00400005;

    int i;
    for(i = 5; i >= 0; i--) {
        max2871_spi_write(i, registers[i]);
        delay_ms(20);
    }
}

static void delay_ms(int ms)
{
	uint32_t i;
    while(ms--) {
        for (i = 0; i < 20000; i++) {
            __asm__("nop");
        }
    }
}


static void serial_delay(void)
{
	uint32_t i;

	for (i = 0; i < 2; i++)
		__asm__("nop");
}


/* SPI register write
 *
 * Send 32 bits:
 *  First 29 bits are data
 *  Last 3 bits are register number
 */
static void max2871_spi_write(uint8_t r, uint32_t v) {

#if DEBUG
	LOG("0x%04x -> reg%d\n", v, r);
#else

	uint32_t bits = 32;
	uint32_t msb = 1 << (bits -1);
	uint32_t data = v | r;

	/* make sure everything is starting in the correct state */
	gpio_set(PORT_VCO_LE, PIN_VCO_LE);
	gpio_clear(PORT_VCO_SCLK, PIN_VCO_SCLK);
	gpio_clear(PORT_VCO_SDATA, PIN_VCO_SDATA);

	/* start transaction by bringing LE low */
	gpio_clear(PORT_VCO_LE, PIN_VCO_LE);

	while (bits--) {
		if (data & msb)
			gpio_set(PORT_VCO_SDATA, PIN_VCO_SDATA);
		else
			gpio_clear(PORT_VCO_SDATA, PIN_VCO_SDATA);
		data <<= 1;

		serial_delay();
		gpio_set(PORT_VCO_SCLK, PIN_VCO_SCLK);

		serial_delay();
		gpio_clear(PORT_VCO_SCLK, PIN_VCO_SCLK);
	}

	gpio_set(PORT_VCO_LE, PIN_VCO_LE);
#endif
}

void max2871_write_registers(void)
{
    int i;
    for(i = 5; i >= 0; i--) {
        max2871_spi_write(i, registers[i]);
    }
}

/* Set frequency (MHz). */
uint64_t mixer_set_frequency(uint16_t mhz)
{
    (void) mhz;
    return mhz;
}

void mixer_tx(void)
{}
void mixer_rx(void)
{}
void mixer_rxtx(void)
{}
void mixer_enable(void)
{}
void mixer_disable(void)
{}
void mixer_set_gpo(uint8_t gpo)
{
    (void) gpo;
}


