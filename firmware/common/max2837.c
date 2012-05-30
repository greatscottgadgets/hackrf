/*
 * 'gcc -DTEST -DDEBUG -O2 -o test max2837.c' prints out what test
 * program would do if it had a real spi library
 *
 * 'gcc -DTEST -DBUS_PIRATE -O2 -o test max2837.c' prints out bus
 * pirate commands to do the same thing.
 */
#include <stdint.h>
#include "max2837.h"
#include "max2837_regs.def" // private register def macros

#if (defined DEBUG || defined BUS_PIRATE)
#include <stdio.h>
#define LOG printf
#else
#define LOG(x,...)
#endif

uint16_t max2837_regs[MAX2837_NUM_REGS] = { 
	0x150,   /* 0 */
	0x002,   /* 1 */
	0x1f4,   /* 2 */
	0x1b9,   /* 3 */
	0x00a,   /* 4 */
	0x080,   /* 5 */
	0x006,   /* 6 */
	0x000,   /* 7 */
	0x080,   /* 8 */
	0x018,   /* 9 */
	0x058,   /* 10 */
	0x016,   /* 11 */
	0x24f,   /* 12 */
	0x150,   /* 13 */
	0x1c5,   /* 14 */
	0x081,   /* 15 */
	0x01c,   /* 16 */
	0x155,   /* 17 */
	0x155,   /* 18 */
	0x153,   /* 19 */
	0x241,   /* 20 */
	0x02c,   /* 21 */
	0x1a9,   /* 22 */
	0x24f,   /* 23 */
	0x180,   /* 24 */
	0x100,   /* 25 */
	0x3ca,   /* 26 */
	0x3e3,   /* 27 */
	0x0c0,   /* 28 */
	0x3f0,   /* 29 */
	0x080,   /* 30 */
	0x000 }; /* 31 */

/* Mark all regsisters dirty so all will be written at init. */
uint32_t max2837_regs_dirty = 0xffffffff;

void max2837_init(void)
{
	LOG("# max2837_init\n");
	/* TODO - reset all register values to defaults? Where from? */
	max2837_regs_dirty = 0xffffffff;

	/* Write default register values to chip. */
	max2837_regs_commit();
}

/* SPI register read. */
uint16_t max2837_spi_read(uint8_t r) {
	return 0;
}

/* SPI register write */
void max2837_spi_write(uint8_t r, uint16_t v) {

#ifdef BUS_PIRATE
	LOG("{0x%02x 0x%02x]\n", 0x80 | ((uint16_t)r<<2) | ((v>>8) & 0x3),
	    v & 0xff);
#endif
#ifdef DEBUG
	LOG("0x%03x -> reg%d\n", v, r);
#endif

}

uint16_t max2837_reg_read(uint8_t r)
{
	if ((max2837_regs_dirty >> r) & 0x1) {
		max2837_spi_read(r);
	};
	return max2837_regs[r];
}

void max2837_reg_write(uint8_t r, uint16_t v)
{
	max2837_regs[r] = v;
	max2837_spi_write(r, v);
	MAX2837_REG_SET_CLEAN(r);
}

void max2837_regs_read(void)
{
	;
}

static inline void max2837_reg_commit(uint8_t r)
{
	max2837_reg_write(r,max2837_regs[r]);
}

void max2837_regs_commit(void)
{
	int r;
	for(r = 0; r < MAX2837_NUM_REGS; r++) {
		if ((max2837_regs_dirty >> r) & 0x1) {
			max2837_reg_commit(r);
		}
	}
}

/* TODO - placeholder */
void max2837_start(void)
{
	LOG("# max2837_start\n");
	set_MAX2837_EN_SPI(1);
	max2837_regs_commit();
}

/* TODO - placeholder */
void max2837_stop(void)
{
	LOG("# max2837_stop\n");
	set_MAX2837_EN_SPI(0);
	max2837_regs_commit();
}

void max2837_set_frequency(uint32_t freq)
{
	uint8_t band;
	uint32_t div_frac;
	uint8_t div_int;

	/* Select band. Allow tuning outside specified bands. */
	if (freq < 2400000000)
		band = MAX2837_LOGEN_BSW_2_3;
	else if (freq < 2500000000)
		band = MAX2837_LOGEN_BSW_2_4;
	else if (freq < 2600000000)
		band = MAX2837_LOGEN_BSW_2_5;
	else
		band = MAX2837_LOGEN_BSW_2_6;

	LOG("# max2837_set_frequency %ld, band %d\n", freq, band);

	/* TODO - frac vs int, compute values */
	div_int = 0x12;
	div_frac = 0x34567;

	/* Write order matters here, so commit INT and FRAC_HI before
	 * committing FRAC_LOG, which is the trigger for VCO
	 * auto-select. TODO - it's cleaner this way, but it would be
	 * faster to explicitly commit the registers explicitly so the
	 * dirty bits aren't scanned twice. */
	set_MAX2837_SYN_INT(div_int);
	set_MAX2837_SYN_FRAC_HI((div_frac >> 10) & 0x3f);
	max2837_regs_commit();
	set_MAX2837_SYN_FRAC_LO(div_frac & 0x3f);
	max2837_regs_commit();
}

#ifdef TEST
uint16_t test(void)
{
	LOG("# test\n");
	uint16_t t = get_MAX2837_Lbias();
	set_MAX2837_Lbias(MAX2837_Lbias_NOMINAL);
	set_MAX2837_Mbias(MAX2837_Mbias_NOMINAL);
	max2837_regs_commit();
	return t;
}

int main(int ac, char **av)
{
	max2837_init();
	max2837_start();
	test();
	max2837_set_frequency(2441000000);
	max2837_stop();
}
#endif //TEST
