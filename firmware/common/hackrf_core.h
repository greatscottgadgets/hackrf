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

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#include "cpld_jtag.h"
#include "i2c_bus.h"
#include "i2c_lpc.h"
#include "max283x.h"
#include "max5864.h"
#include "mixer.h"
#include "platform_detect.h" // IWYU pragma: keep
#include "radio.h"
#include "rf_path.h"
#include "sgpio.h"
#include "si5351c.h"
#include "spi_ssp.h"
#include "w25q80bv.h"
#ifdef IS_PRALINE
	#include "fpga.h"
	#include "ice40_spi.h"
#endif

/* TODO: Hide these configurations */
extern const i2c_lpc_config_t i2c_config_si5351c_fast_clock;
extern si5351c_driver_t clock_gen;
extern ssp_config_t ssp_config_w25q80bv;

extern max283x_driver_t max283x;
#ifdef IS_PRALINE
extern ice40_spi_driver_t ice40;
extern fpga_driver_t fpga;
#endif
extern max283x_driver_t max283x;
extern max5864_driver_t max5864;
extern mixer_driver_t mixer;
extern w25q80bv_driver_t spi_flash;
extern sgpio_config_t sgpio_config;
extern radio_t radio;
extern rf_path_t rf_path;
extern jtag_t jtag_cpld;
extern i2c_bus_t i2c0;

void ssp1_set_mode_max283x(void);
void ssp1_set_mode_max5864(void);
#ifdef IS_PRALINE
void ssp1_set_mode_ice40(void);
#endif

void pin_shutdown(void);
void pin_setup(void);

#ifdef IS_PRALINE
void enable_1v2_power(void);
void disable_1v2_power(void);
void enable_3v3aux_power(void);
void disable_3v3aux_power(void);
#endif
void enable_1v8_power(void);
void disable_1v8_power(void);

#if defined(IS_RAD1O) || defined(IS_HACKRF_ONE) || defined(IS_PRALINE)
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

#ifdef IS_PRALINE
typedef enum {
	CLKIN_SIGNAL_P1 = 0,
	CLKIN_SIGNAL_P22 = 1,
} clkin_signal_t;

void narrowband_filter_set(const uint8_t value);
void clkin_ctrl_set(const clkin_signal_t value);
#endif

#ifdef __cplusplus
}
#endif
