/*
 * Copyright 2019-2022 Great Scott Gadgets <info@greatscottgadgets.com>
 * Copyright 2019 Dominic Spill
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

#include "ui_rad1o.h"

#include "rad1o/display.h"
#include "rad1o/draw.h"
#include "rad1o/print.h"
#include "rad1o/render.h"
#include "rad1o/smallfonts.h"
#include "rad1o/ubuntu18.h"

#include <stdio.h>

static uint64_t freq = 0;
static uint32_t sample_rate = 0;
static uint32_t filter_bw = 0;
static rf_path_direction_t direction;
static uint32_t bblna_gain = 0;
static uint32_t bbvga_gain = 0;
static uint32_t bbtxvga_gain = 0;
static bool lna_on = false;
static transceiver_mode_t trx_mode;
static bool enabled = false;

#define BLACK      0b00000000
#define RED        0b11100000
#define RED_DARK   0b01100000
#define GREEN      0b00011100
#define GREEN_DARK 0b00001100
#define BLUE       0b00000011
#define WHITE      0b11111111
#define GREY       0b01001101

static void draw_frequency(void)
{
	char tmp[100];
	uint32_t mhz;
	uint32_t khz;

	mhz = freq / 1000000;
	khz = (freq - mhz * 1000000) / 1000;

	rad1o_setTextColor(BLACK, GREEN);
	rad1o_setIntFont(&Font_Ubuntu18pt);
	sprintf(tmp, "%4u.%03u", (unsigned int) mhz, (unsigned int) khz);
	rad1o_lcdPrint(tmp);

	rad1o_setIntFont(&Font_7x8);
	rad1o_lcdMoveCrsr(1, 18 - 7);
	rad1o_lcdPrint("MHz");
}

static void draw_tx_rx(void)
{
	uint8_t bg, fg;

	rad1o_setIntFont(&Font_Ubuntu18pt);

	bg = BLACK;

	fg = GREY;
	if (direction == RF_PATH_DIRECTION_OFF) {
		fg = WHITE;
	}
	rad1o_setTextColor(bg, fg);
	rad1o_lcdPrint("OFF ");

	fg = GREY;
	if (direction == RF_PATH_DIRECTION_RX) {
		fg = GREEN;
	}
	rad1o_setTextColor(bg, fg);
	rad1o_lcdPrint("RX ");

	fg = GREY;
	if (direction == RF_PATH_DIRECTION_TX) {
		fg = RED;
	}
	rad1o_setTextColor(bg, fg);
	rad1o_lcdPrint("TX");

	rad1o_setIntFont(&Font_7x8);
}

static void ui_update(void)
{
	char tmp[100];
	uint32_t mhz;
	uint32_t khz;

	if (!enabled) {
		return;
	}

	rad1o_lcdClear();
	rad1o_lcdFill(0x00);

	rad1o_drawHLine(0, 0, RESX - 1, WHITE);
	rad1o_drawVLine(0, 0, RESY - 1, WHITE);

	rad1o_drawHLine(RESY - 1, 0, RESX - 1, WHITE);
	rad1o_drawVLine(RESX - 1, 0, RESY - 1, WHITE);

	rad1o_lcdSetCrsr(25, 2);

	rad1o_setTextColor(BLACK, GREEN);

	rad1o_lcdPrint("HackRF Mode");
	rad1o_lcdNl();

	rad1o_drawHLine(11, 0, RESX - 1, WHITE);

	rad1o_lcdSetCrsr(2, 12);
	if (trx_mode == TRANSCEIVER_MODE_RX_SWEEP) {
		rad1o_setIntFont(&Font_Ubuntu18pt);
		rad1o_lcdPrint("SWEEP");
	} else {
		draw_frequency();
	}

	rad1o_drawHLine(40, 0, RESX - 1, WHITE);

	rad1o_lcdSetCrsr(6, 41);
	draw_tx_rx();
	rad1o_drawHLine(69, 0, RESX - 1, WHITE);

	rad1o_setTextColor(BLACK, WHITE);
	rad1o_lcdSetCrsr(2, 71);
	rad1o_lcdPrint("Rate:   ");
	mhz = sample_rate / 1000000;
	khz = (sample_rate - mhz * 1000000) / 1000;
	sprintf(tmp, "%2u.%03u MHz", (unsigned int) mhz, (unsigned int) khz);
	rad1o_lcdPrint(tmp);
	rad1o_lcdNl();

	rad1o_lcdMoveCrsr(2, 0);
	rad1o_lcdPrint("Filter: ");
	mhz = filter_bw / 1000000;
	khz = (filter_bw - mhz * 1000000) / 1000;
	sprintf(tmp, "%2u.%03u MHz", (unsigned int) mhz, (unsigned int) khz);
	rad1o_lcdPrint(tmp);
	rad1o_lcdNl();

	rad1o_drawHLine(88, 0, RESX - 1, WHITE);

	rad1o_setTextColor(BLACK, WHITE);
	rad1o_lcdSetCrsr(2, 90);
	rad1o_lcdPrint("      Gains");
	rad1o_lcdNl();

	rad1o_setTextColor(BLACK, GREEN);
	rad1o_lcdMoveCrsr(2, 2);
	rad1o_lcdPrint("AMP: ");
	if (lna_on) {
		rad1o_setTextColor(BLACK, RED);
		rad1o_lcdPrint("ON ");
	} else {
		rad1o_lcdPrint("OFF");
	}

	rad1o_setTextColor(BLACK, RED_DARK);
	if (direction == RF_PATH_DIRECTION_TX) {
		rad1o_setTextColor(BLACK, RED);
	}
	sprintf(tmp, " TX: %u dB", (unsigned int) bbtxvga_gain);
	rad1o_lcdPrint(tmp);
	rad1o_lcdNl();

	rad1o_lcdMoveCrsr(2, 0);
	rad1o_setTextColor(BLACK, GREEN_DARK);
	if (direction == RF_PATH_DIRECTION_RX) {
		rad1o_setTextColor(BLACK, GREEN);
	}
	sprintf(tmp, "LNA: %2u dB", (unsigned int) bblna_gain);
	rad1o_lcdPrint(tmp);
	rad1o_lcdNl();
	rad1o_lcdMoveCrsr(2, 0);
	sprintf(tmp, "VGA: %2u dB", (unsigned int) bbvga_gain);
	rad1o_lcdPrint(tmp);
	rad1o_lcdNl();

	rad1o_lcdDisplay();

	// Don't ask...
	ssp1_set_mode_max283x();
}

static void rad1o_ui_init(void)
{
	rad1o_lcdInit();
	enabled = true;
	ui_update();
}

static void rad1o_ui_deinit(void)
{
	rad1o_lcdDeInit();
	enabled = false;
	// Don't ask...
	ssp1_set_mode_max283x();
}

static void rad1o_ui_set_frequency(uint64_t frequency)
{
	freq = frequency;

	if (TRANSCEIVER_MODE_RX_SWEEP == trx_mode) {
	} else {
		ui_update();
	}
}

static void rad1o_ui_set_sample_rate(uint32_t _sample_rate)
{
	sample_rate = _sample_rate;
	ui_update();
}

static void rad1o_ui_set_direction(const rf_path_direction_t _direction)
{
	direction = _direction;
	ui_update();
}

static void rad1o_ui_set_filter_bw(uint32_t bandwidth)
{
	filter_bw = bandwidth;
	ui_update();
}

static void rad1o_ui_set_lna_power(bool _lna_on)
{
	lna_on = _lna_on;
	ui_update();
}

static void rad1o_ui_set_bb_lna_gain(const uint32_t gain_db)
{
	bblna_gain = gain_db;
	ui_update();
}

static void rad1o_ui_set_bb_vga_gain(const uint32_t gain_db)
{
	bbvga_gain = gain_db;
	ui_update();
}

static void rad1o_ui_set_bb_tx_vga_gain(const uint32_t gain_db)
{
	bbtxvga_gain = gain_db;
	ui_update();
}

static void rad1o_ui_set_first_if_frequency(
	const uint64_t frequency __attribute__((unused)))
{
	// Not implemented
}

static void rad1o_ui_set_filter(const rf_path_filter_t filter __attribute__((unused)))
{
	// Not implemented
}

static void rad1o_ui_set_antenna_bias(bool antenna_bias __attribute__((unused)))
{
	// Not implemented
}

static void rad1o_ui_set_clock_source(clock_source_t source __attribute__((unused)))
{
	// Not implemented
}

static void rad1o_ui_set_transceiver_mode(transceiver_mode_t mode)
{
	trx_mode = mode;
	ui_update();
}

static bool rad1o_ui_operacake_gpio_compatible(void)
{
	return true;
}

static const hackrf_ui_t rad1o_ui = {
	&rad1o_ui_init,
	&rad1o_ui_deinit,
	&rad1o_ui_set_frequency,
	&rad1o_ui_set_sample_rate,
	&rad1o_ui_set_direction,
	&rad1o_ui_set_filter_bw,
	&rad1o_ui_set_lna_power,
	&rad1o_ui_set_bb_lna_gain,
	&rad1o_ui_set_bb_vga_gain,
	&rad1o_ui_set_bb_tx_vga_gain,
	&rad1o_ui_set_first_if_frequency,
	&rad1o_ui_set_filter,
	&rad1o_ui_set_antenna_bias,
	&rad1o_ui_set_clock_source,
	&rad1o_ui_set_transceiver_mode,
	&rad1o_ui_operacake_gpio_compatible,
};

const hackrf_ui_t* rad1o_ui_setup(void)
{
	return &rad1o_ui;
}
