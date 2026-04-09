/*
 * Copyright 2012-2022 Great Scott Gadgets <info@greatscottgadgets.com>
 * Copyright 2012 Will Code <willcode4@gmail.com>
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

#include <string.h>

#include "max283x.h"

#include "platform_detect.h"
#include "platform_gpio.h"
#include "spi_bus.h"

#if defined(PRALINE) || defined(UNIVERSAL)
	#include "max2831_target.h"
#endif
#if !defined(PRALINE) || defined(UNIVERSAL)
	#include "max2837_target.h"
#endif
#if defined(HACKRF_ONE) || defined(UNIVERSAL)
	#include "max2839_target.h"
#endif

extern spi_bus_t spi_bus_ssp1;

#if defined(PRALINE) || defined(UNIVERSAL)
max2831_driver_t max2831 = {
	.bus = &spi_bus_ssp1,
	.target_init = max2831_target_init,
	.set_mode = max2831_target_set_mode,
};
#endif

#if !defined(PRALINE) || defined(UNIVERSAL)
max2837_driver_t max2837 = {
	.bus = &spi_bus_ssp1,
	.target_init = max2837_target_init,
	.set_mode = max2837_target_set_mode,
};
#endif

#if defined(HACKRF_ONE) || defined(UNIVERSAL)
max2839_driver_t max2839 = {
	.bus = &spi_bus_ssp1,
	.target_init = max2839_target_init,
	.set_mode = max2839_target_set_mode,
};
#endif

/* Initialize chip. */
void max283x_setup(max283x_driver_t* const drv)
{
	const platform_gpio_t* gpio = platform_gpio();

	/* MAX283x GPIO PinMux */
#ifdef IS_PRALINE
	if (IS_PRALINE) {
		drv->type = MAX2831_VARIANT;
		max2831.gpio_enable = gpio->max283x_enable;
		max2831.gpio_rxtx = gpio->max283x_rx_enable;
		max2831.gpio_rxhp = gpio->max2831_rxhp;
		max2831.gpio_ld = gpio->max2831_ld;
		memcpy(&drv->drv.max2831, &max2831, sizeof(max2831));
		max2831_setup(&drv->drv.max2831);
	}
#endif
#ifdef IS_NOT_PRALINE
	if (IS_NOT_PRALINE) {
	#ifdef IS_H1_R9
		if (IS_H1_R9) {
			drv->type = MAX2839_VARIANT;
			max2839.gpio_enable = gpio->max283x_enable;
			max2839.gpio_rxtx = gpio->max283x_rx_enable;
			memcpy(&drv->drv.max2839, &max2839, sizeof(max2839));
			max2839_setup(&drv->drv.max2839);
		}
	#endif
	#ifdef IS_NOT_H1_R9
		if (IS_NOT_H1_R9) {
			drv->type = MAX2837_VARIANT;
			max2837.gpio_enable = gpio->max283x_enable;
			max2837.gpio_rx_enable = gpio->max283x_rx_enable;
			max2837.gpio_tx_enable = gpio->max283x_tx_enable;
			memcpy(&drv->drv.max2837, &max2837, sizeof(max2837));
			max2837_setup(&drv->drv.max2837);
		}
	#endif
	}
#endif
}

/* Macros to simplify dispatch of variant-specific operations. */

/* clang-format off */

#if defined(UNIVERSAL)
	// UNIVERSAL: all variants
	#define DISPATCH(_drv, _max2831, _max2837, _max2839) \
		if (_drv->type == MAX2831_VARIANT) { \
			_max2831; \
		} else if (_drv->type == MAX2837_VARIANT) { \
			_max2837; \
		} else { \
			_max2839; \
		}
#elif defined(HACKRF_ONE)
	// HACKRF_ONE: MAX2837 and MAX2839 only
	#define DISPATCH(_drv, _max2831, _max2837, _max2839) \
		if (_drv->type == MAX2837_VARIANT) { \
			_max2837; \
		} else { \
			_max2839; \
		}
#elif defined(PRALINE)
	// PRALINE: MAX2831 only
	#define DISPATCH(_drv, _max2831, _max2837, _max2839) \
		(void) drv; \
		_max2831
#else
	// JAWBREAKER, RAD1O: MAX2837 only
	#define DISPATCH(_drv, _max2831, _max2837, _max2839) \
		(void) drv; \
		_max2837
#endif

#define CALL(_drv, _func, ...) \
	DISPATCH(drv, \
		max2831_ ## _func(&_drv->drv.max2831, ##__VA_ARGS__), \
		max2837_ ## _func(&_drv->drv.max2837, ##__VA_ARGS__), \
		max2839_ ## _func(&_drv->drv.max2839, ##__VA_ARGS__) \
	); \

#define RESULT(_drv, _type, _func, ...) ({ \
	_type _result; \
	DISPATCH(drv, \
		_result = max2831_ ## _func(&_drv->drv.max2831, ##__VA_ARGS__), \
		_result = max2837_ ## _func(&_drv->drv.max2837, ##__VA_ARGS__), \
		_result = max2839_ ## _func(&_drv->drv.max2839, ##__VA_ARGS__) \
	); \
	_result; \
})

#define CONSTANT(_drv, _type, _name) ({\
	_type value; \
	DISPATCH(drv, \
		value = MAX2831_ ## _name, \
		value = MAX2837_ ## _name, \
		value = MAX2839_ ## _name \
	); \
	value; \
})

#define PRALINE_ONLY(_drv, _max2831) DISPATCH(_drv, _max2831, , )

/* clang-format on */

/* Returns the number of registers supported by the driver. */
uint16_t max283x_num_regs(max283x_driver_t* const drv)
{
	return CONSTANT(drv, uint16_t, NUM_REGS);
}

/* Returns the maximum data register value supported by the driver. */
uint16_t max283x_data_regs_max_value(max283x_driver_t* const drv)
{
	return CONSTANT(drv, uint16_t, DATA_REGS_MAX_VALUE);
}

/* Read a register via SPI. Save a copy to memory and return
 * value. Mark clean. */
uint16_t max283x_reg_read(max283x_driver_t* const drv, uint8_t r)
{
	return RESULT(drv, uint16_t, reg_read, r);
}

/* Write value to register via SPI and save a copy to memory. Mark
 * clean. */
void max283x_reg_write(max283x_driver_t* const drv, uint8_t r, uint16_t v)
{
	CALL(drv, reg_write, r, v);
}

/* Write all dirty registers via SPI from memory. Mark all clean. Some
 * operations require registers to be written in a certain order. Use
 * provided routines for those operations. */
void max283x_regs_commit(max283x_driver_t* const drv)
{
	CALL(drv, regs_commit);
}

void max283x_set_mode(max283x_driver_t* const drv, const max283x_mode_t new_mode)
{
	DISPATCH(
		drv,
		max2831_set_mode(&drv->drv.max2831, (max2831_mode_t) new_mode),
		max2837_set_mode(&drv->drv.max2837, (max2837_mode_t) new_mode),
		max2839_set_mode(&drv->drv.max2839, (max2839_mode_t) new_mode));
}

max283x_mode_t max283x_mode(max283x_driver_t* const drv)
{
	DISPATCH(
		drv,
		return (max283x_mode_t) max2831_mode(&drv->drv.max2831),
		return (max283x_mode_t) max2837_mode(&drv->drv.max2837),
		return (max283x_mode_t) max2839_mode(&drv->drv.max2839));
}

/* Turn on/off all chip functions. Does not control oscillator and CLKOUT */
void max283x_start(max283x_driver_t* const drv)
{
	CALL(drv, start);
}

void max283x_stop(max283x_driver_t* const drv)
{
	CALL(drv, stop);
}

/* Set frequency in Hz. Frequency setting is a multi-step function
 * where order of register writes matters. */
void max283x_set_frequency(max283x_driver_t* const drv, uint32_t freq)
{
	CALL(drv, set_frequency, freq);
}

uint32_t max283x_set_lpf_bandwidth(
	max283x_driver_t* const drv,
	const max283x_mode_t mode,
	const uint32_t bandwidth_hz)
{
#if !defined(PRALINE)
	(void) mode;
#endif
	DISPATCH(
		drv,
		return max2831_set_lpf_bandwidth(
			&drv->drv.max2831,
			(max2831_mode_t) mode,
			bandwidth_hz),
		return max2837_set_lpf_bandwidth(&drv->drv.max2837, bandwidth_hz),
		return max2839_set_lpf_bandwidth(&drv->drv.max2839, bandwidth_hz));
}

bool max283x_set_lna_gain(max283x_driver_t* const drv, const uint32_t gain_db)
{
	return RESULT(drv, bool, set_lna_gain, gain_db);
}

bool max283x_set_vga_gain(max283x_driver_t* const drv, const uint32_t gain_db)
{
	return RESULT(drv, bool, set_vga_gain, gain_db);
}

bool max283x_set_txvga_gain(max283x_driver_t* const drv, const uint32_t gain_db)
{
	return RESULT(drv, bool, set_txvga_gain, gain_db);
}

void max283x_tx(max283x_driver_t* const drv)
{
	CALL(drv, tx);
}

void max283x_rx(max283x_driver_t* const drv)
{
	CALL(drv, rx);
}

/* Set MAX2831 receiver high-pass filter corner frequency in Hz */
void max283x_set_rx_hpf_frequency(
	max283x_driver_t* const drv,
	const max283x_rx_hpf_freq_t freq)
{
#if !defined(PRALINE) && !defined(UNIVERSAL)
	(void) freq;
#endif
	PRALINE_ONLY(
		drv,
		max2831_set_rx_hpf_frequency(
			&drv->drv.max2831,
			(max2831_rx_hpf_freq_t) freq));
}

void max283x_tx_calibration(max283x_driver_t* const drv)
{
	PRALINE_ONLY(drv, max2831_tx_calibration(&drv->drv.max2831));
}

void max283x_rx_calibration(max283x_driver_t* const drv)
{
	PRALINE_ONLY(drv, max2831_rx_calibration(&drv->drv.max2831));
}
