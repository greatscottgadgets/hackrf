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
 * 'gcc -DTEST -DDEBUG -O2 -o test max2837.c' prints out what test
 * program would do if it had a real spi library
 *
 * 'gcc -DTEST -DBUS_PIRATE -O2 -o test max2837.c' prints out bus
 * pirate commands to do the same thing.
 */

#include <stdint.h>
#include <string.h>
#include "max2837.h"
#include "max2837_regs.def" // private register def macros

/* Default register values. */
static const uint16_t max2837_regs_default[MAX2837_NUM_REGS] = { 
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
	/*
	 * Charge Pump Common Mode Enable bit (0) of register 21 must be set or TX
	 * does not work.  Page 1 of the SPI doc says not to set it (0x02c), but
	 * page 21 says it should be set by default (0x02d).
	 */
	0x02d,   /* 21 */
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

/* Set up all registers according to defaults specified in docs. */
static void max2837_init(max2837_driver_t* const drv)
{
	drv->target_init(drv);
	max2837_set_mode(drv, MAX2837_MODE_SHUTDOWN);

	memcpy(drv->regs, max2837_regs_default, sizeof(drv->regs));
	drv->regs_dirty = 0xffffffff;

	/* Write default register values to chip. */
	max2837_regs_commit(drv);
}

/*
 * Set up pins for GPIO and SPI control, configure SSP peripheral for SPI, and
 * set our own default register configuration.
 */
void max2837_setup(max2837_driver_t* const drv)
{
	max2837_init(drv);
	
	/* Use SPI control instead of B1-B7 pins for gain settings. */
	set_MAX2837_TXVGA_GAIN_SPI_EN(drv, 1);
	set_MAX2837_TXVGA_GAIN_MSB_SPI_EN(drv, 1);
	//set_MAX2837_TXVGA_GAIN(0x3f); /* maximum attenuation */
	set_MAX2837_TXVGA_GAIN(drv, 0x00); /* minimum attenuation */
	set_MAX2837_VGAMUX_enable(drv, 1);
	set_MAX2837_VGA_EN(drv, 1);
	set_MAX2837_HPC_RXGAIN_EN(drv, 0);
	set_MAX2837_HPC_STOP(drv, MAX2837_STOP_1K);
	set_MAX2837_LNAgain_SPI_EN(drv, 1);
	set_MAX2837_LNAgain(drv, MAX2837_LNAgain_MAX); /* maximum gain */
	set_MAX2837_VGAgain_SPI_EN(drv, 1);
	set_MAX2837_VGA(drv, 0x18); /* reasonable gain for noisy 2.4GHz environment */

	/* maximum rx output common-mode voltage */
	set_MAX2837_BUFF_VCM(drv, MAX2837_BUFF_VCM_1_25);

	/* configure baseband filter for 8 MHz TX */
	set_MAX2837_LPF_EN(drv, 1);
	set_MAX2837_ModeCtrl(drv, MAX2837_ModeCtrl_RxLPF);
	set_MAX2837_FT(drv, MAX2837_FT_5M);

	max2837_regs_commit(drv);
}

static uint16_t max2837_read(max2837_driver_t* const drv, uint8_t r) {
	uint16_t value = (1 << 15) | (r << 10);
	spi_bus_transfer(drv->bus, &value, 1);
	return value & 0x3ff;
}

static void max2837_write(max2837_driver_t* const drv, uint8_t r, uint16_t v) {
	uint16_t value = (r << 10) | (v & 0x3ff);
	spi_bus_transfer(drv->bus, &value, 1);
}

uint16_t max2837_reg_read(max2837_driver_t* const drv, uint8_t r)
{
	if ((drv->regs_dirty >> r) & 0x1) {
		drv->regs[r] = max2837_read(drv, r);
	};
	return drv->regs[r];
}

void max2837_reg_write(max2837_driver_t* const drv, uint8_t r, uint16_t v)
{
	drv->regs[r] = v;
	max2837_write(drv, r, v);
	MAX2837_REG_SET_CLEAN(drv, r);
}

static inline void max2837_reg_commit(max2837_driver_t* const drv, uint8_t r)
{
	max2837_reg_write(drv, r, drv->regs[r]);
}

void max2837_regs_commit(max2837_driver_t* const drv)
{
	int r;
	for(r = 0; r < MAX2837_NUM_REGS; r++) {
		if ((drv->regs_dirty >> r) & 0x1) {
			max2837_reg_commit(drv, r);
		}
	}
}

void max2837_set_mode(max2837_driver_t* const drv, const max2837_mode_t new_mode) {
	drv->set_mode(drv, new_mode);
}

max2837_mode_t max2837_mode(max2837_driver_t* const drv) {
	return drv->mode;
}

void max2837_start(max2837_driver_t* const drv)
{
	set_MAX2837_EN_SPI(drv, 1);
	max2837_regs_commit(drv);
	max2837_set_mode(drv, MAX2837_MODE_STANDBY);
}

void max2837_tx(max2837_driver_t* const drv)
{
	set_MAX2837_ModeCtrl(drv, MAX2837_ModeCtrl_TxLPF);
	max2837_regs_commit(drv);
	max2837_set_mode(drv, MAX2837_MODE_TX);
}

void max2837_rx(max2837_driver_t* const drv)
{
	set_MAX2837_ModeCtrl(drv, MAX2837_ModeCtrl_RxLPF);
	max2837_regs_commit(drv);
	max2837_set_mode(drv, MAX2837_MODE_RX);
}

void max2837_stop(max2837_driver_t* const drv)
{
	set_MAX2837_EN_SPI(drv, 0);
	max2837_regs_commit(drv);
	max2837_set_mode(drv, MAX2837_MODE_SHUTDOWN);
}

void max2837_set_frequency(max2837_driver_t* const drv, uint32_t freq)
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
		band = MAX2837_LOGEN_BSW_2_3;
		lna_band = MAX2837_LNAband_2_4;
	}
	else if (freq < 2500000000U) {
		band = MAX2837_LOGEN_BSW_2_4;
		lna_band = MAX2837_LNAband_2_4;
	}
	else if (freq < 2600000000U) {
		band = MAX2837_LOGEN_BSW_2_5;
		lna_band = MAX2837_LNAband_2_6;
	}
	else {
		band = MAX2837_LOGEN_BSW_2_6;
		lna_band = MAX2837_LNAband_2_6;
	}

	/* ASSUME 40MHz PLL. Ratio = F*(4/3)/40,000,000 = F/30,000,000 */
	div_int = freq / 30000000;
	div_rem = freq % 30000000;
	div_frac = 0;
	div_cmp = 30000000;
	for( i = 0; i < 20; i++) {
		div_frac <<= 1;
		div_cmp >>= 1;
		if (div_rem > div_cmp) {
			div_frac |= 0x1;
			div_rem -= div_cmp;
		}
	}

	/* Band settings */
	set_MAX2837_LOGEN_BSW(drv, band);
	set_MAX2837_LNAband(drv, lna_band);

	/* Write order matters here, so commit INT and FRAC_HI before
	 * committing FRAC_LO, which is the trigger for VCO
	 * auto-select. TODO - it's cleaner this way, but it would be
	 * faster to explicitly commit the registers explicitly so the
	 * dirty bits aren't scanned twice. */
	set_MAX2837_SYN_INT(drv, div_int);
	set_MAX2837_SYN_FRAC_HI(drv, (div_frac >> 10) & 0x3ff);
	max2837_regs_commit(drv);
	set_MAX2837_SYN_FRAC_LO(drv, div_frac & 0x3ff);
	max2837_regs_commit(drv);
}

typedef struct {
	uint32_t bandwidth_hz;
	uint32_t ft;
} max2837_ft_t;

static const max2837_ft_t max2837_ft[] = {
	{  1750000, MAX2837_FT_1_75M },
	{  2500000, MAX2837_FT_2_5M },
	{  3500000, MAX2837_FT_3_5M },
	{  5000000, MAX2837_FT_5M },
	{  5500000, MAX2837_FT_5_5M },
	{  6000000, MAX2837_FT_6M },
	{  7000000, MAX2837_FT_7M },
	{  8000000, MAX2837_FT_8M },
	{  9000000, MAX2837_FT_9M },
	{ 10000000, MAX2837_FT_10M },
	{ 12000000, MAX2837_FT_12M },
	{ 14000000, MAX2837_FT_14M },
	{ 15000000, MAX2837_FT_15M },
	{ 20000000, MAX2837_FT_20M },
	{ 24000000, MAX2837_FT_24M },
	{ 28000000, MAX2837_FT_28M },
	{        0, 0 },
};

bool max2837_set_lpf_bandwidth(max2837_driver_t* const drv, const uint32_t bandwidth_hz) {
	const max2837_ft_t* p = max2837_ft;
	while( p->bandwidth_hz != 0 ) {
		if( p->bandwidth_hz >= bandwidth_hz ) {
			break;
		}
		p++;
	}
	
	if( p->bandwidth_hz != 0 ) {
		set_MAX2837_FT(drv, p->ft);
		max2837_regs_commit(drv);
		return true;
	} else {
		return false;
	}
}

bool max2837_set_lna_gain(max2837_driver_t* const drv, const uint32_t gain_db) {
	uint16_t val;
	switch(gain_db){
		case 40:
			val = MAX2837_LNAgain_MAX;
			break;
		case 32:
			val = MAX2837_LNAgain_M8;
			break;
		case 24:
			val = MAX2837_LNAgain_M16;
			break;
		case 16:
			val = MAX2837_LNAgain_M24;
			break;
		case 8:
			val = MAX2837_LNAgain_M32;
			break;
		case 0:
			val = MAX2837_LNAgain_M40;
			break;
		default:
			return false;
	}
	set_MAX2837_LNAgain(drv, val);
	max2837_reg_commit(drv, 1);
	return true;
}

bool max2837_set_vga_gain(max2837_driver_t* const drv, const uint32_t gain_db) {
	if( (gain_db & 0x1) || gain_db > 62)/* 0b11111*2 */
		return false;
		
	set_MAX2837_VGA(drv, 31-(gain_db >> 1) );
	max2837_reg_commit(drv, 5);
	return true;
}

bool max2837_set_txvga_gain(max2837_driver_t* const drv, const uint32_t gain_db) {
	uint16_t val=0;
	if(gain_db <16){
		val = 31-gain_db;
		val |= (1 << 5); // bit6: 16db
	} else{
		val = 31-(gain_db-16);
	}
	
	set_MAX2837_TXVGA_GAIN(drv, val);
	max2837_reg_commit(drv, 29);
	return true;
}
