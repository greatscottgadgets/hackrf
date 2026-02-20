/*
 * Copyright 2012-2026 Great Scott Gadgets <info@greatscottgadgets.com>
 * Copyright 2012 Benjamin Vernoux <titanmkd@gmail.com>
 * Copyright 2012 Jared Boone <jared@sharebrained.com>
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

#ifndef __HACKRF_CORE_H
#define __HACKRF_CORE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#include "si5351c.h"
#include "spi_ssp.h"

#include "max283x.h"
#include "max5864.h"
#include "mixer.h"
#include "w25q80bv.h"
#include "sgpio.h"
#include "radio.h"
#include "rf_path.h"
#include "cpld_jtag.h"
#include "ice40_spi.h"
#include "fpga.h"

#include "platform_scu.h"

/* TODO: Hide these configurations */
extern si5351c_driver_t clock_gen;
extern ssp_config_t ssp_config_w25q80bv;

extern max283x_driver_t max283x;
#if defined(PRALINE) || defined(HACKRF_ALL)
extern ice40_spi_driver_t ice40;
extern fpga_driver_t fpga;
#endif
extern max5864_driver_t max5864;
extern mixer_driver_t mixer;
extern w25q80bv_driver_t spi_flash;
extern sgpio_config_t sgpio_config;
extern radio_t radio;
extern rf_path_t rf_path;
extern jtag_t jtag_cpld;
extern i2c_bus_t i2c0;

void cpu_clock_init(void);
void clock_gen_init(void);
void clock_gen_shutdown(void);
void ssp1_set_mode_max283x(void);
void ssp1_set_mode_max5864(void);
#if defined(PRALINE) || defined(HACKRF_ALL)
void ssp1_set_mode_ice40(void);
#endif

void pin_shutdown(void);
void pin_setup(void);

#if defined(PRALINE) || defined(HACKRF_ALL)
void enable_1v2_power(void);
void disable_1v2_power(void);
void enable_3v3aux_power(void);
void disable_3v3aux_power(void);
#endif
void enable_1v8_power(void);
void disable_1v8_power(void);

bool sample_rate_frac_set(uint32_t rate_num, uint32_t rate_denom);
bool sample_rate_set(const uint32_t sampling_rate_hz);

clock_source_t activate_best_clock_source(void);

#if defined(RAD1O) || defined(HACKRF_ONE) || defined(PRALINE) || defined(HACKRF_ALL)
void enable_rf_power(void);
void disable_rf_power(void);
#endif

typedef enum {
	LED1 = 0,
	LED2 = 1,
	LED3 = 2,
	LED4 = 3,
} led_t;

void led_on(const led_t led);
void led_off(const led_t led);
void led_toggle(const led_t led);
void set_leds(const uint8_t state);

void trigger_enable(const bool enable);

void halt_and_flash(const uint32_t duration);

#if defined(PRALINE) || defined(HACKRF_ALL)
typedef enum {
	P1_SIGNAL_TRIGGER_IN = 0,
	P1_SIGNAL_AUX_CLK1 = 1,
	P1_SIGNAL_CLKIN = 2,
	P1_SIGNAL_TRIGGER_OUT = 3,
	P1_SIGNAL_P22_CLKIN = 4,
	P1_SIGNAL_P2_5 = 5,
	P1_SIGNAL_NC = 6,
	P1_SIGNAL_AUX_CLK2 = 7,
} p1_ctrl_signal_t;

typedef enum {
	P2_SIGNAL_CLK3 = 0,
	P2_SIGNAL_TRIGGER_IN = 2,
	P2_SIGNAL_TRIGGER_OUT = 3,
} p2_ctrl_signal_t;

typedef enum {
	CLKIN_SIGNAL_P1 = 0,
	CLKIN_SIGNAL_P22 = 1,
} clkin_signal_t;

void p1_ctrl_set(const p1_ctrl_signal_t signal);
void p2_ctrl_set(const p2_ctrl_signal_t signal);
void narrowband_filter_set(const uint8_t value);
void clkin_ctrl_set(const clkin_signal_t value);
void pps_out_set(const uint8_t value);
#endif

#ifdef __cplusplus
}
#endif

#endif /* __HACKRF_CORE_H */
