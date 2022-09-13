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
	0x000, /* 0 */
	0x00C, /* 1 */
	0x080, /* 2 */
	0x1b9, /* 3 */
	0x3e6, /* 4 */
	0x100, /* 5 */
	0x000, /* 6 */
	0x208, /* 7 */
	0x220, /* 8 */
	0x018, /* 9 */
	0x00c, /* 10 */
	0x004, /* 11 */
	0x24f, /* 12 */
	0x150, /* 13 */
	0x3c5, /* 14 */
	0x201, /* 15 */
	0x01c, /* 16 */
	0x155, /* 17 */
	0x155, /* 18 */
	0x153, /* 19 */
	0x249, /* 20 */
	/*
	 * Charge Pump Common Mode Enable bit (0) of register 21 must be set or TX
	 * does not work.  Page 1 of the SPI doc says not to set it (0x02c), but
	 * page 21 says it should be set by default (0x02d).
	 */
	0x02d,  /* 21 */
	0x1a9,  /* 22 */
	0x24f,  /* 23 */
	0x180,  /* 24 */
	0x000,  /* 25 */
	0x3c0,  /* 26 */
	0x200,  /* 27 */
	0x0c0,  /* 28 */
	0x03f,  /* 29 */
	0x380,  /* 30 */
	0x340}; /* 31 */

/* Set up all registers according to defaults specified in docs. */
static void max2839_init(max2839_driver_t* const drv)
{
	drv->target_init(drv);
	max2839_set_mode(drv, MAX2839_MODE_SHUTDOWN);

	memcpy(drv->regs, max2839_regs_default, sizeof(drv->regs));
	drv->regs_dirty = 0xffffffff;

	/* Write default register values to chip. */
	max2837_regs_commit(drv);
}

/*
 * Set up pins for GPIO and SPI control, configure SSP peripheral for SPI, and
 * set our own default register configuration.
 */
void max2839_setup(max2839_driver_t* const drv)
{
	max2839_init(drv);
	// TODO
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
	if ((drv->regs_dirty >> r) & 0x1) {
		drv->regs[r] = max2839_read(drv, r);
	};
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
	set_MAX2839_EN_SPI(drv, 1);
	max2839_regs_commit(drv);
	max2839_set_mode(drv, MAX2839_MODE_STANDBY);
}

void max2839_tx(max2839_driver_t* const drv)
{
	// TODO
	max2839_regs_commit(drv);
	max2839_set_mode(drv, MAX2839_MODE_TX);
}

void max2839_rx(max2839_driver_t* const drv)
{
	// TODO
	max2839_regs_commit(drv);
	max2839_set_mode(drv, MAX2839_MODE_RX);
}

void max2839_stop(max2839_driver_t* const drv)
{
	// TODO
	max2839_regs_commit(drv);
	max2839_set_mode(drv, MAX2839_MODE_SHUTDOWN);
}

void max2839_set_frequency(max2839_driver_t* const drv, uint32_t freq)
{
	// TODO
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
	// TODO: validate steps here
	switch(gain_db){
		case 40:
			val = MAX2839_LNAgain_MAX;
			break;
		case 32:
			val = MAX2839_LNAgain_M8;
			break;
		case 24:
			val = MAX2839_LNAgain_M16;
			break;
		case 8:
			val = MAX2839_LNAgain_M32;
			break;
		default:
			return false;
	}
	set_MAX2839_LNAgain(drv, val);
	max2839_reg_commit(drv, 1);
	return true;
}

bool max2839_set_vga_gain(max2839_driver_t* const drv, const uint32_t gain_db) {
	if( (gain_db & 0x1) || gain_db > 62) {/* 0b11111*2 */
		return false;
}

	set_MAX2839_VGA(drv, 31-(gain_db >> 1) );
	max2839_reg_commit(drv, 5);
	return true;
}

bool max2839_set_txvga_gain(max2839_driver_t* const drv, const uint32_t gain_db) {
	uint16_t val=0;
	if(gain_db <16){
		val = 31-gain_db;
		val |= (1 << 5); // bit6: 16db
	} else{
		val = 31-(gain_db-16);
	}

	set_MAX2839_TXVGA_GAIN(drv, val);
	max2839_reg_commit(drv, 29);
	return true;
}
