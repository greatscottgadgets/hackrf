/*
 * Copyright 2012 Will Code? (TODO: Proper attribution)
 * Copyright 2014 Jared Boone <jared@sharebrained.com>
 *
 * This file is part of HackRF.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

/*
 * 'gcc -DTEST -DDEBUG -O2 -o test max2839.c' prints out what test
 * program would do if it had a real spi library
 *
 * 'gcc -DTEST -DBUS_PIRATE -O2 -o test max2839.c' prints out bus
 * pirate commands to do the same thing.
 */

#include <stdint.h>
#include <string.h>
#include "max2839.h"
#include "max2839_regs.def" // private register def macros

/* Default register values. */
static const uint16_t max2839_regs_default[MAX2839_NUM_REGS] = {
	0x000,  /* 0 */
	0x00c,  /* 1:  data sheet says 0x00c but read 0x22c */
	0x080,  /* 2 */
	0x1b9,  /* 3:  data sheet says 0x1b9 but read 0x1b0 */
	0x3e6,  /* 4 */
	0x100,  /* 5 */
	0x000,  /* 6 */
	0x208,  /* 7 */
	0x220,  /* 8:  data sheet says 0x220 but read 0x000 */
	0x018,  /* 9 */
	0x00c,  /* 10 */
	0x004,  /* 11: data sheet says 0x004 but read 0x000 */
	0x24f,  /* 12 */
	0x150,  /* 13 */
	0x3c5,  /* 14 */
	0x201,  /* 15 */
	0x01c,  /* 16 */
	0x155,  /* 17 */
	0x155,  /* 18 */
	0x153,  /* 19 */
	0x249,  /* 20 */
	0x02d,  /* 21: data sheet says 0x02d but read 0x13d */
	0x1a9,  /* 22 */
	0x24f,  /* 23 */
	0x180,  /* 24 */
	0x000,  /* 25: data sheet says 0x000 but read 0x00a */
	0x3c0,  /* 26 */
	0x200,  /* 27: data sheet says 0x200 but read 0x22a */
	0x0c0,  /* 28 */
	0x03f,  /* 29: data sheet says 0x03f but read 0x07f */
	0x300,  /* 30: data sheet says 0x300 but read 0x398 */
	0x340}; /* 31: data sheet says 0x340 but read 0x359 */

/*
 * All of the discrepancies listed above are in fields that either don't matter
 * or are undocumented except "set to recommended value". We set them to the
 * data sheet defaults even though the inital part we tested started up with
 * different settings.
 */

/* Set up all registers according to defaults specified in docs. */
static void max2839_init(max2839_driver_t* const drv)
{
	drv->target_init(drv);
	max2839_set_mode(drv, MAX2839_MODE_SHUTDOWN);

	memcpy(drv->regs, max2839_regs_default, sizeof(drv->regs));
	drv->regs_dirty = 0xffffffff;

	/* Write default register values to chip. */
	max2839_regs_commit(drv);
}

/*
 * Set up pins for GPIO and SPI control, configure SSP peripheral for SPI, and
 * set our own default register configuration.
 */
void max2839_setup(max2839_driver_t* const drv)
{
	max2839_init(drv);

	/* Use SPI control instead of B0-B7 pins for gain settings. */
	set_MAX2839_LNAgain_SPI(drv, 1);
	set_MAX2839_VGAgain_SPI(drv, 1);
	set_MAX2839_TX_VGA_Gain_SPI(drv, 1);

	/* enable RXINB */
	set_MAX2839_MIMO_SELECT(drv, 1);

	/* set gains for unused RXINA path to minimum */
	set_MAX2839_LNA1gain(drv, MAX2839_LNA1gain_M32);
	set_MAX2839_Rx1_VGAgain(drv, 0x3f);

	/* set maximum RX output common-mode voltage */
	set_MAX2839_RX_VCM(drv, MAX2839_RX_VCM_1_35);

	/* set HPF corner frequency to 1 kHz */
	set_MAX2839_HPC_STOP(drv, MAX2839_STOP_1K);

	max2839_regs_commit(drv);
}

static uint16_t max2839_read(max2839_driver_t* const drv, uint8_t r)
{
	uint16_t value = (1 << 15) | (r << 10);
	spi_bus_transfer(drv->bus, &value, 1);
	return value & 0x3ff;
}

static void max2839_write(max2839_driver_t* const drv, uint8_t r, uint16_t v)
{
	uint16_t value = (r << 10) | (v & 0x3ff);
	spi_bus_transfer(drv->bus, &value, 1);
}

uint16_t max2839_reg_read(max2839_driver_t* const drv, uint8_t r)
{
	// always read actual value from SPI for now
	//if ((drv->regs_dirty >> r) & 0x1) {
	drv->regs[r] = max2839_read(drv, r);
	//};
	return drv->regs[r];
}

void max2839_reg_write(max2839_driver_t* const drv, uint8_t r, uint16_t v)
{
	drv->regs[r] = v;
	max2839_write(drv, r, v);
	MAX2839_REG_SET_CLEAN(drv, r);
}

static inline void max2839_reg_commit(max2839_driver_t* const drv, uint8_t r)
{
	max2839_reg_write(drv, r, drv->regs[r]);
}

void max2839_regs_commit(max2839_driver_t* const drv)
{
	int r;
	for (r = 0; r < MAX2839_NUM_REGS; r++) {
		if ((drv->regs_dirty >> r) & 0x1) {
			max2839_reg_commit(drv, r);
		}
	}
}

void max2839_set_mode(max2839_driver_t* const drv, const max2839_mode_t new_mode)
{
	drv->set_mode(drv, new_mode);
}

max2839_mode_t max2839_mode(max2839_driver_t* const drv)
{
	return drv->mode;
}

void max2839_start(max2839_driver_t* const drv)
{
	set_MAX2839_chip_enable(drv, 1);
	max2839_regs_commit(drv);
	max2839_set_mode(drv, MAX2839_MODE_STANDBY);
}

void max2839_tx(max2839_driver_t* const drv)
{
	// FIXME does this do anything without LPFmode_SPI set?
	// do we need it to?
	set_MAX2839_LPFmode(drv, MAX2839_LPFmode_TxLPF);
	max2839_regs_commit(drv);
	max2839_set_mode(drv, MAX2839_MODE_TX);
}

void max2839_rx(max2839_driver_t* const drv)
{
	set_MAX2839_LPFmode(drv, MAX2839_LPFmode_RxLPF);
	max2839_regs_commit(drv);
	max2839_set_mode(drv, MAX2839_MODE_RX);
}

void max2839_stop(max2839_driver_t* const drv)
{
	set_MAX2839_chip_enable(drv, 0);
	max2839_regs_commit(drv);
	max2839_set_mode(drv, MAX2839_MODE_SHUTDOWN);
}

void max2839_set_frequency(max2839_driver_t* const drv, uint32_t freq)
{
	uint8_t band;
	uint8_t lna_band;
	uint32_t div_frac;
	uint32_t div_int;
	uint32_t div_rem;
	uint32_t div_cmp;
	int i;

	/* Select band. Allow tuning outside specified bands. */
	if (freq < 2400000000U) {
		band = MAX2839_LOGEN_BSW_2_3;
		lna_band = MAX2839_LNAband_2_4;
	} else if (freq < 2500000000U) {
		band = MAX2839_LOGEN_BSW_2_4;
		lna_band = MAX2839_LNAband_2_4;
	} else if (freq < 2600000000U) {
		band = MAX2839_LOGEN_BSW_2_5;
		lna_band = MAX2839_LNAband_2_6;
	} else {
		band = MAX2839_LOGEN_BSW_2_6;
		lna_band = MAX2839_LNAband_2_6;
	}

	/* ASSUME 40MHz PLL. Ratio = F*(4/3)/40,000,000 = F/30,000,000 */
	div_int = freq / 30000000;
	div_rem = freq % 30000000;
	div_frac = 0;
	div_cmp = 30000000;
	for (i = 0; i < 20; i++) {
		div_frac <<= 1;
		div_cmp >>= 1;
		if (div_rem > div_cmp) {
			div_frac |= 0x1;
			div_rem -= div_cmp;
		}
	}

	/* Band settings */
	set_MAX2839_LOGEN_BSW(drv, band);
	set_MAX2839_LNAband(drv, lna_band);

	/* Write order matters here, so commit INT and FRAC_HI before
	 * committing FRAC_LO, which is the trigger for VCO
	 * auto-select. TODO - it's cleaner this way, but it would be
	 * faster to explicitly commit the registers explicitly so the
	 * dirty bits aren't scanned twice. */
	set_MAX2839_SYN_INT(drv, div_int);
	set_MAX2839_SYN_FRAC_HI(drv, (div_frac >> 10) & 0x3ff);
	max2839_regs_commit(drv);
	set_MAX2839_SYN_FRAC_LO(drv, div_frac & 0x3ff);
	max2839_regs_commit(drv);
}

typedef struct {
	uint32_t bandwidth_hz;
	uint32_t ft;
} max2839_ft_t;

// clang-format off
static const max2839_ft_t max2839_ft[] = {
	{  1750000, MAX2839_FT_1_75M },
	{  2500000, MAX2839_FT_2_5M },
	{  3500000, MAX2839_FT_3_5M },
	{  5000000, MAX2839_FT_5M },
	{  5500000, MAX2839_FT_5_5M },
	{  6000000, MAX2839_FT_6M },
	{  7000000, MAX2839_FT_7M },
	{  8000000, MAX2839_FT_8M },
	{  9000000, MAX2839_FT_9M },
	{ 10000000, MAX2839_FT_10M },
	{ 12000000, MAX2839_FT_12M },
	{ 14000000, MAX2839_FT_14M },
	{ 15000000, MAX2839_FT_15M },
	{ 20000000, MAX2839_FT_20M },
	{ 24000000, MAX2839_FT_24M },
	{ 28000000, MAX2839_FT_28M },
	{        0, 0 },
};
//clang-format on

uint32_t max2839_set_lpf_bandwidth(max2839_driver_t* const drv, const uint32_t bandwidth_hz) {
	const max2839_ft_t* p = max2839_ft;
	while( p->bandwidth_hz != 0 ) {
		if( p->bandwidth_hz >= bandwidth_hz ) {
			break;
		}
		p++;
	}

	if( p->bandwidth_hz != 0 ) {
		set_MAX2839_FT(drv, p->ft);
		max2839_regs_commit(drv);
	}

	return p->bandwidth_hz;
}

bool max2839_set_lna_gain(max2839_driver_t* const drv, const uint32_t gain_db) {
	uint16_t val;
	switch(gain_db){
		case 40:
			val = MAX2839_LNA2gain_MAX;
			break;
		case 32:
			val = MAX2839_LNA2gain_M8;
			break;
		case 24:
			// FIXME correct missing settings with VGA adjustment?
		case 16:
			val = MAX2839_LNA2gain_M16;
			break;
		case 8:
		case 0:
			val = MAX2839_LNA2gain_M32;
			break;
		default:
			return false;
	}
	set_MAX2839_LNA2gain(drv, val);
	max2839_reg_commit(drv, 6);
	return true;
}

bool max2839_set_vga_gain(max2839_driver_t* const drv, const uint32_t gain_db) {
	if( (gain_db & 0x1) || gain_db > 62) {/* 0b11111*2 */
		return false;
}

	set_MAX2839_Rx2_VGAgain(drv, (63-gain_db));
	max2839_reg_commit(drv, 6);
	return true;
}

bool max2839_set_txvga_gain(max2839_driver_t* const drv, const uint32_t gain_db)
{
	uint16_t val = 0;
	val = 47 - gain_db;

	set_MAX2839_TX_VGA_GAIN(drv, val);
	max2839_reg_commit(drv, 29);
	return true;
}
