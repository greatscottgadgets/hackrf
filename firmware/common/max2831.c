/*
 * Copyright 2025 Great Scott Gadgets <info@greatscottgadgets.com>
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
 * 'gcc -DTEST -DDEBUG -O2 -o test max2831.c' prints out what test
 * program would do if it had a real spi library
 *
 * 'gcc -DTEST -DBUS_PIRATE -O2 -o test max2831.c' prints out bus
 * pirate commands to do the same thing.
 */

#include <stdint.h>
#include <string.h>
#include "max2831.h"
#include "max2831_regs.def" // private register def macros
#include "selftest.h"
#include "adc.h"

#define MIN(x, y) ((x) < (y) ? (x) : (y))

/* Default register values. */
static const uint16_t max2831_regs_default[MAX2831_NUM_REGS] = {
	0x1740, /* 0: enable fractional mode (Table 16 recommends 0x0740, clearing unknown bit) */
	0x119a, /* 1 */
	0x1003, /* 2 */
	0x0079, /* 3: PLL divider settings for 2437 MHz */
	0x3666, /* 4: PLL divider settings for 2437 MHz */
	0x00a4, /* 5: divide reference frequency by 2 */
	0x0060, /* 6: enable TX power detector */
	0x1022, /* 7: 110% TX LPF bandwidth */
	0x2021, /* 8: pin control of RX gain, 11 MHz LPF bandwidth */
	0x03b5, /* 9: pin control of TX gain */
	0x1d80, /* 10: 3.5 us PA enable delay, zero PA bias */
	0x0074, /* 11: LNA high gain, RX VGA moderate gain (Table 27 recommends 0x007f, maximum gain) */
	0x0140, /* 12: TX VGA minimum */
	0x0e92, /* 13 */
	0x0100, /* 14: reference clock output disabled */
	0x0145, /* 15: RX IQ common mode 1.1 V */
};

/* Set up all registers according to defaults specified in docs. */
static void max2831_init(max2831_driver_t* const drv)
{
	drv->target_init(drv);
	max2831_set_mode(drv, MAX2831_MODE_SHUTDOWN);

	memcpy(drv->regs, max2831_regs_default, sizeof(drv->regs));
	drv->regs_dirty = 0xffff;

	/* Write default register values to chip. */
	max2831_regs_commit(drv);
}

/*
 * Set up pins for GPIO and SPI control, configure SSP peripheral for SPI, and
 * set our own default register configuration.
 */
void max2831_setup(max2831_driver_t* const drv)
{
	max2831_init(drv);

	/* Use SPI control instead of B1-B7 pins for gain settings. */
	set_MAX2831_RXVGA_GAIN_SPI_EN(drv, 1);
	set_MAX2831_TXVGA_GAIN_SPI_EN(drv, 1);

	//set_MAX2831_TXVGA_GAIN(0x3f); /* maximum gain */
	set_MAX2831_TXVGA_GAIN(drv, 0x00); /* minimum gain */
	//set_MAX2831_RX_HPF_SEL(drv, MAX2831_RX_HPF_100_HZ);
	set_MAX2831_LNA_GAIN(drv, MAX2831_LNA_GAIN_MAX); /* maximum gain */
	set_MAX2831_RXVGA_GAIN(drv, 0x18);

	/* maximum rx output common-mode voltage */
	//set_MAX2831_RXIQ_VCM(drv, MAX2831_RXIQ_VCM_1_2);

	/* configure baseband filter for 8 MHz TX */
	set_MAX2831_LPF_COARSE(drv, MAX2831_RX_LPF_7_5M);
	set_MAX2831_RX_LPF_FINE_ADJ(drv, MAX2831_RX_LPF_FINE_100);
	set_MAX2831_TX_LPF_FINE_ADJ(drv, MAX2831_TX_LPF_FINE_100);

	/* clock output disable */
	set_MAX2831_CLKOUT_PIN_EN(drv, 0);

	max2831_regs_commit(drv);
}

static void max2831_write(max2831_driver_t* const drv, uint8_t r, uint16_t v)
{
	uint32_t word = (((uint32_t) v & 0x3fff) << 4) | (r & 0xf);
	uint16_t values[2] = {word >> 9, word & 0x1ff};
	spi_bus_transfer(drv->bus, values, 2);
}

uint16_t max2831_reg_read(max2831_driver_t* const drv, uint8_t r)
{
	return drv->regs[r];
}

void max2831_reg_write(max2831_driver_t* const drv, uint8_t r, uint16_t v)
{
	drv->regs[r] = v;
	max2831_write(drv, r, v);
	MAX2831_REG_SET_CLEAN(drv, r);
}

static inline void max2831_reg_commit(max2831_driver_t* const drv, uint8_t r)
{
	max2831_reg_write(drv, r, drv->regs[r]);
}

void max2831_regs_commit(max2831_driver_t* const drv)
{
	int r;
	for (r = 0; r < MAX2831_NUM_REGS; r++) {
		if ((drv->regs_dirty >> r) & 0x1) {
			max2831_reg_commit(drv, r);
		}
	}
}

void max2831_set_mode(max2831_driver_t* const drv, const max2831_mode_t new_mode)
{
	// Only change calibration bits if necessary to reduce SPI activity.
	bool tx_cal = (new_mode == MAX2831_MODE_TX_CALIBRATION);
	bool rx_cal = (new_mode == MAX2831_MODE_RX_CALIBRATION);
	if (get_MAX2831_TX_CAL_MODE_EN(drv) != tx_cal) {
		set_MAX2831_TX_CAL_MODE_EN(drv, tx_cal);
		max2831_regs_commit(drv);
	}
	if (get_MAX2831_RX_CAL_MODE_EN(drv) != rx_cal) {
		set_MAX2831_RX_CAL_MODE_EN(drv, rx_cal);
		max2831_regs_commit(drv);
	}

	drv->set_mode(drv, new_mode);
	max2831_set_lpf_bandwidth(drv, drv->desired_lpf_bw);
}

max2831_mode_t max2831_mode(max2831_driver_t* const drv)
{
	return drv->mode;
}

void max2831_start(max2831_driver_t* const drv)
{
	max2831_regs_commit(drv);
	max2831_set_mode(drv, MAX2831_MODE_STANDBY);

	/* Read RSSI with ADC. */
	uint16_t rssi_1 = selftest.max2831_mux_rssi_1 = adc_read(1);

	/* Switch to temperature sensor. */
	set_MAX2831_RSSI_MUX(drv, MAX2831_RSSI_MUX_TEMP);
	max2831_regs_commit(drv);

	/* Read temperature. */
	uint16_t temp = selftest.max2831_mux_temp = adc_read(1);

	/* Switch back to RSSI. */
	set_MAX2831_RSSI_MUX(drv, MAX2831_RSSI_MUX_RSSI);
	max2831_regs_commit(drv);

	/* Read RSSI again. */
	uint16_t rssi_2 = selftest.max2831_mux_rssi_2 = adc_read(1);

	/* If the ADC results are as expected, we know our writes are working. */
	bool rssi_1_good = (rssi_1 < 10);
	bool rssi_2_good = (rssi_2 < 10);
	bool temp_good = (temp > 100) && (temp < 500); // -40 to +85C

	selftest.max2831_mux_test_ok = rssi_1_good & rssi_2_good & temp_good;

	if (!selftest.max2831_mux_test_ok) {
		selftest.report.pass = false;
	}
}

void max2831_tx(max2831_driver_t* const drv)
{
	max2831_regs_commit(drv);
	max2831_set_mode(drv, MAX2831_MODE_TX);
}

void max2831_rx(max2831_driver_t* const drv)
{
	max2831_regs_commit(drv);
	max2831_set_mode(drv, MAX2831_MODE_RX);
}

void max2831_tx_calibration(max2831_driver_t* const drv)
{
	max2831_regs_commit(drv);
	max2831_set_mode(drv, MAX2831_MODE_TX_CALIBRATION);
}

void max2831_rx_calibration(max2831_driver_t* const drv)
{
	max2831_regs_commit(drv);
	max2831_set_mode(drv, MAX2831_MODE_RX_CALIBRATION);
}

void max2831_stop(max2831_driver_t* const drv)
{
	max2831_regs_commit(drv);
	max2831_set_mode(drv, MAX2831_MODE_SHUTDOWN);
}

void max2831_set_frequency(max2831_driver_t* const drv, uint32_t freq)
{
	uint32_t div_frac;
	uint32_t div_int;
	uint32_t div_rem;
	uint32_t div_cmp;
	int i;

	/* ASSUME 40MHz PLL. Ratio = F*R/40,000,000. */
	/* TODO: fixed to R=2. Check if it's worth exploring R=1. */
	freq += (20000000 >> 21); /* round to nearest frequency */
	div_int = freq / 20000000;
	div_rem = freq % 20000000;
	div_frac = 0;
	div_cmp = 20000000;
	for (i = 0; i < 20; i++) {
		div_frac <<= 1;
		div_rem <<= 1;
		if (div_rem >= div_cmp) {
			div_frac |= 0x1;
			div_rem -= div_cmp;
		}
	}

	/* Write order matters? */
	//set_MAX2831_SYN_REF_DIV(drv, MAX2831_SYN_REF_DIV_2);
	set_MAX2831_SYN_INT(drv, div_int);
	set_MAX2831_SYN_FRAC_HI(drv, (div_frac >> 6) & 0x3fff);
	set_MAX2831_SYN_FRAC_LO(drv, div_frac & 0x3f);
	max2831_regs_commit(drv);
}

typedef struct {
	uint32_t bandwidth_hz;
	uint8_t ft;
} max2831_ft_t;

typedef struct {
	uint8_t percent;
	uint8_t ft_fine;
} max2831_ft_fine_t;

// clang-format off
/* measured -0.5 dB complex baseband bandwidth for each register setting */
static const max2831_ft_t max2831_rx_ft[] = {
	{ 11600000, MAX2831_RX_LPF_7_5M },
	{ 15100000, MAX2831_RX_LPF_8_5M },
	{ 22600000, MAX2831_RX_LPF_15M },
	{ 28300000, MAX2831_RX_LPF_18M },
	{        0, 0 },
};

static const max2831_ft_fine_t max2831_rx_ft_fine[] = {
	{  90, MAX2831_RX_LPF_FINE_90 },
	{  95, MAX2831_RX_LPF_FINE_95 },
	{ 100, MAX2831_RX_LPF_FINE_100 },
	{ 105, MAX2831_RX_LPF_FINE_105 },
	{ 110, MAX2831_RX_LPF_FINE_110 },
	{   0, 0 },
};

/* measured -0.5 dB complex baseband bandwidth for each register setting */
static const max2831_ft_t max2831_tx_ft[] = {
	{ 11900000, MAX2831_TX_LPF_8M },
	{ 15800000, MAX2831_TX_LPF_11M },
	{ 23600000, MAX2831_TX_LPF_16_5M },
	{ 31300000, MAX2831_TX_LPF_22_5M },
	{        0, 0 },
};

static const max2831_ft_fine_t max2831_tx_ft_fine[] = {
	{  90, MAX2831_TX_LPF_FINE_90 },
	{  95, MAX2831_TX_LPF_FINE_95 },
	{ 100, MAX2831_TX_LPF_FINE_100 },
	{ 105, MAX2831_TX_LPF_FINE_105 },
	{ 110, MAX2831_TX_LPF_FINE_110 },
	{ 115, MAX2831_TX_LPF_FINE_115 },
	{   0, 0 },
};
//clang-format on


uint32_t max2831_set_lpf_bandwidth(max2831_driver_t* const drv, const uint32_t bandwidth_hz) {
	const max2831_ft_t* coarse;
	const max2831_ft_fine_t* fine;

	drv->desired_lpf_bw = bandwidth_hz;

	if (drv->mode == MAX2831_MODE_RX) {
		coarse = max2831_rx_ft;
		fine = max2831_rx_ft_fine;
	} else {
		coarse = max2831_tx_ft;
		fine = max2831_tx_ft_fine;
	}

	/* Find coarse and fine settings for LPF. */
	bool found = false;
	const max2831_ft_fine_t* f = fine;
	for (; coarse->bandwidth_hz != 0; coarse++) {
		uint32_t coarse_aux = coarse->bandwidth_hz / 100;
		for (f = fine; f->percent != 0; f++) {
			if ((coarse_aux * f->percent) >= drv->desired_lpf_bw) {
				found = true;
				break;
			}
		}
		if (found) break;
	}

	/*
	 * Use the widest setting if a wider bandwidth than our maximum is
	 * requested.
	 */
	if (!found) {
		coarse--;
		f--;
	}

	/* Program found settings. */
	set_MAX2831_LPF_COARSE(drv, coarse->ft);
	if (drv->mode == MAX2831_MODE_RX) {
		set_MAX2831_RX_LPF_FINE_ADJ(drv, f->ft_fine);
	} else {
		set_MAX2831_TX_LPF_FINE_ADJ(drv, f->ft_fine);
	}
	max2831_regs_commit(drv);

	return coarse->bandwidth_hz * f->percent / 100;
}

bool max2831_set_lna_gain(max2831_driver_t* const drv, const uint32_t gain_db) {
	uint16_t val;
	switch(gain_db){
		case 40:  // MAX2837 compatibility
		case 33:
		case 32:  // MAX2837 compatibility
			val = MAX2831_LNA_GAIN_MAX;
			break;
		case 24:  // MAX2837 compatibility
		case 16:
			val = MAX2831_LNA_GAIN_M16;
			break;
		case 8:	  // MAX2837 compatibility
		case 0:
			val = MAX2831_LNA_GAIN_M33;
			break;
		default:
			return false;
	}
	set_MAX2831_LNA_GAIN(drv, val);
	max2831_reg_commit(drv, 11);
	return true;
}

bool max2831_set_vga_gain(max2831_driver_t* const drv, const uint32_t gain_db) {
	if( (gain_db & 0x1) || gain_db > 62) {/* 0b11111*2 */
		return false;
	}
	
	set_MAX2831_RXVGA_GAIN(drv, (gain_db >> 1) );
	max2831_reg_commit(drv, 11);
	return true;
}

bool max2831_set_txvga_gain(max2831_driver_t* const drv, const uint32_t gain_db) {	
	uint16_t value = MIN((gain_db << 1) | 1, 0x3f);
	set_MAX2831_TXVGA_GAIN(drv, value);
	max2831_reg_commit(drv, 12);
	return true;
}
