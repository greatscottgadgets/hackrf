/*
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

#include "rad1o_display.h"
#include "rad1o_print.h"
#include "rad1o_draw.h"
#include "rad1o_render.h"
#include "rad1o_smallfonts.h"
#include "rad1o_ubuntu18.h"

#include "ui_rad1o.h"

#include <stdio.h>

/* Weak functions from rad1o app */
void hackrf_ui_init(void) __attribute__((weak));
void hackrf_ui_setFrequency(uint64_t _freq) __attribute__((weak));
void hackrf_ui_setSampleRate(uint32_t _sample_rate) __attribute__((weak));
void hackrf_ui_setDirection(const rf_path_direction_t _direction) __attribute__((weak));
void hackrf_ui_setFilterBW(uint32_t bw) __attribute__((weak));
void hackrf_ui_setLNAPower(bool _lna_on) __attribute__((weak));
void hackrf_ui_setBBLNAGain(const uint32_t gain_db) __attribute__((weak));
void hackrf_ui_setBBVGAGain(const uint32_t gain_db) __attribute__((weak));
void hackrf_ui_setBBTXVGAGain(const uint32_t gain_db) __attribute__((weak));
void hackrf_ui_setFirstIFFrequency(const uint64_t freq) __attribute__((weak));
void hackrf_ui_setFilter(const rf_path_filter_t filter) __attribute__((weak));
void hackrf_ui_setAntennaBias(bool antenna_bias) __attribute__((weak));
void hackrf_ui_setClockSource(clock_source_t source) __attribute__((weak));

uint64_t freq = 0;
uint32_t sample_rate = 0;
uint32_t filter_bw = 0;
rf_path_direction_t direction;
uint32_t bblna_gain = 0;
uint32_t bbvga_gain = 0;
uint32_t bbtxvga_gain = 0;
bool lna_on = false;
bool enabled = false;

#define BLACK       0b00000000
#define RED         0b11100000
#define RED_DARK    0b01100000
#define GREEN       0b00011100
#define GREEN_DARK  0b00001100
#define BLUE        0b00000011
#define WHITE       0b11111111
#define GREY        0b01001101

void draw_frequency(void)
{
    char tmp[100];
    uint32_t mhz;
    uint32_t khz;

    mhz = freq/1e6;
    khz = (freq - mhz * 1e6) / 1000;

    setTextColor(BLACK, GREEN);
    setIntFont(&Font_Ubuntu18pt);
    sprintf(tmp, "%4u.%03u", (unsigned int)mhz, (unsigned int)khz); lcdPrint(tmp);

    setIntFont(&Font_7x8);
    lcdMoveCrsr(1, 18-7);
    lcdPrint("MHz");
}

void draw_tx_rx(void)
{
    uint8_t bg, fg;

    setIntFont(&Font_Ubuntu18pt);

    bg = BLACK;

    fg = GREY;
    if(direction == RF_PATH_DIRECTION_OFF) {
        fg = WHITE;
    }
    setTextColor(bg, fg);
    lcdPrint("OFF ");

    fg = GREY;
    if(direction == RF_PATH_DIRECTION_RX) {
        fg = GREEN;
    }
    setTextColor(bg, fg);
    lcdPrint("RX ");

    fg = GREY;
    if(direction == RF_PATH_DIRECTION_TX) {
        fg = RED;
    }
    setTextColor(bg, fg);
    lcdPrint("TX");

    setIntFont(&Font_7x8);
}


void hackrf_ui_update(void)
{
    char tmp[100];
    uint32_t mhz;
    uint32_t khz;

    if(!enabled) {
        return;
    }

    lcdClear();
    lcdFill(0x00);

    drawHLine(0, 0, RESX - 1, WHITE);
    drawVLine(0, 0, RESY - 1, WHITE);

    drawHLine(RESY - 1, 0, RESX - 1, WHITE);
    drawVLine(RESX - 1, 0, RESY - 1, WHITE);

    lcdSetCrsr(25, 2);

    setTextColor(BLACK, GREEN);

    lcdPrint("HackRF Mode");lcdNl();

    drawHLine(11, 0, RESX - 1, WHITE);

    lcdSetCrsr(2, 12);
    draw_frequency();

    drawHLine(40, 0, RESX - 1, WHITE);

    lcdSetCrsr(6, 41);
    draw_tx_rx();
    drawHLine(69, 0, RESX - 1, WHITE);

    setTextColor(BLACK, WHITE);
    lcdSetCrsr(2, 71);
    lcdPrint("Rate:   ");
    mhz = sample_rate/1e6;
    khz = (sample_rate - mhz * 1e6) / 1000;
    sprintf(tmp, "%2u.%03u MHz", (unsigned int)mhz, (unsigned int)khz); lcdPrint(tmp); lcdNl();

    lcdMoveCrsr(2, 0);
    lcdPrint("Filter: ");
    mhz = filter_bw/1e6;
    khz = (filter_bw - mhz * 1e6) / 1000;
    sprintf(tmp, "%2u.%03u MHz", (unsigned int)mhz, (unsigned int)khz); lcdPrint(tmp); lcdNl();

    drawHLine(88, 0, RESX - 1, WHITE);

    setTextColor(BLACK, WHITE);
    lcdSetCrsr(2, 90);
    lcdPrint("      Gains"); lcdNl();

    setTextColor(BLACK, GREEN);
    lcdMoveCrsr(2, 2);
    lcdPrint("AMP: ");
    if(lna_on) {
        setTextColor(BLACK, RED);
        lcdPrint("ON ");
    } else {
        lcdPrint("OFF");
    }

    setTextColor(BLACK, RED_DARK);
    if(direction == RF_PATH_DIRECTION_TX) {
        setTextColor(BLACK, RED);
    }
    sprintf(tmp, " TX: %u dB", (unsigned int)bbtxvga_gain); lcdPrint(tmp); lcdNl();

    lcdMoveCrsr(2, 0);
    setTextColor(BLACK, GREEN_DARK);
    if(direction == RF_PATH_DIRECTION_RX) {
        setTextColor(BLACK, GREEN);
    }
    sprintf(tmp, "LNA: %2u dB", (unsigned int)bblna_gain); lcdPrint(tmp); lcdNl();
    lcdMoveCrsr(2, 0);
    sprintf(tmp, "VGA: %2u dB", (unsigned int)bbvga_gain); lcdPrint(tmp); lcdNl();

    lcdDisplay();

    // Don't ask...
    ssp1_set_mode_max2837();
}


static void rad1o_ui_init(void) {
    lcdInit();
    enabled = true;
    hackrf_ui_update();
}

static void rad1o_ui_deinit(void) {
    lcdDeInit();
    enabled = false;
    // Don't ask...
    ssp1_set_mode_max2837();
}

static void rad1o_ui_set_frequency(uint64_t frequency) {
    freq = frequency;
    hackrf_ui_update();
}

static void rad1o_ui_set_sample_rate(uint32_t _sample_rate) {
    sample_rate = _sample_rate;
    hackrf_ui_update();
}

static void rad1o_ui_set_direction(const rf_path_direction_t _direction) {
    direction = _direction;
    hackrf_ui_update();
}

static void rad1o_ui_set_filter_bw(uint32_t bandwidth) {
    filter_bw = bandwidth;
    hackrf_ui_update();
}

static void rad1o_ui_set_lna_power(bool _lna_on) {
    lna_on = _lna_on;
    hackrf_ui_update();
}

static void rad1o_ui_set_bb_lna_gain(const uint32_t gain_db) {
    bblna_gain = gain_db;
    hackrf_ui_update();
}

static void rad1o_ui_set_bb_vga_gain(const uint32_t gain_db) {
    bbvga_gain = gain_db;
    hackrf_ui_update();
}

static void rad1o_ui_set_bb_tx_vga_gain(const uint32_t gain_db) {
    bbtxvga_gain = gain_db;
    hackrf_ui_update();
}

static void rad1o_ui_set_first_if_frequency(const uint64_t frequency) {
	hackrf_ui_setFirstIFFrequency(frequency);
}

static void rad1o_ui_set_filter(const rf_path_filter_t filter) {
    hackrf_ui_setFilter(filter);
}

static void rad1o_ui_set_antenna_bias(bool antenna_bias) {
    hackrf_ui_setAntennaBias(antenna_bias);
}

static void rad1o_ui_set_clock_source(clock_source_t source) {
	hackrf_ui_setClockSource(source);
}

static bool rad1o_ui_operacake_gpio_compatible(void) {
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
	&rad1o_ui_operacake_gpio_compatible,
};

const hackrf_ui_t* rad1o_ui_setup(void) {
	return &rad1o_ui;
}
