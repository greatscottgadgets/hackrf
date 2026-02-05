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

#include "max283x.h"

#include "platform_gpio.h"
#include "spi_bus.h"

extern spi_bus_t spi_bus_ssp1;

max2831_driver_t max2831 = {
	.bus = &spi_bus_ssp1,
	.target_init = max2831_target_init,
	.set_mode = max2831_target_set_mode,
};

max2837_driver_t max2837 = {
	.bus = &spi_bus_ssp1,
	.target_init = max2837_target_init,
	.set_mode = max2837_target_set_mode,
};

max2839_driver_t max2839 = {
	.bus = &spi_bus_ssp1,
	.target_init = max2839_target_init,
	.set_mode = max2839_target_set_mode,
};

/* Initialize chip. */
void max283x_setup(max283x_driver_t* const drv, max283x_variant_t type)
{
	drv->type = type;

	const platform_gpio_t* gpio = platform_gpio();

	/* MAX283x GPIO PinMux */
	max2831.gpio_enable = gpio->max283x_enable;
	max2831.gpio_rxtx = gpio->max283x_rx_enable;
	max2831.gpio_rxhp = gpio->max2831_rxhp;
	max2831.gpio_ld = gpio->max2831_ld;
	max2837.gpio_enable = gpio->max283x_enable;
	max2837.gpio_rx_enable = gpio->max283x_rx_enable;
	max2837.gpio_tx_enable = gpio->max283x_tx_enable;
	max2839.gpio_enable = gpio->max283x_enable;
	max2839.gpio_rxtx = gpio->max283x_rx_enable;

	switch (drv->type) {
	case MAX2831_VARIANT:
		memcpy(&drv->drv.max2831, &max2831, sizeof(max2831));
		max2831_setup(&drv->drv.max2831);
		break;

	case MAX2837_VARIANT:
		memcpy(&drv->drv.max2837, &max2837, sizeof(max2837));
		max2837_setup(&drv->drv.max2837);
		break;

	case MAX2839_VARIANT:
		memcpy(&drv->drv.max2839, &max2839, sizeof(max2839));
		max2839_setup(&drv->drv.max2839);
		break;
	}
}

/* Read a register via SPI. Save a copy to memory and return
 * value. Mark clean. */
uint16_t max283x_reg_read(max283x_driver_t* const drv, uint8_t r)
{
	switch (drv->type) {
	case MAX2831_VARIANT:
		return max2831_reg_read(&drv->drv.max2831, r);
		break;

	case MAX2837_VARIANT:
		return max2837_reg_read(&drv->drv.max2837, r);
		break;

	case MAX2839_VARIANT:
		return max2839_reg_read(&drv->drv.max2839, r);
		break;
	}

	return 0;
}

/* Write value to register via SPI and save a copy to memory. Mark
 * clean. */
void max283x_reg_write(max283x_driver_t* const drv, uint8_t r, uint16_t v)
{
	switch (drv->type) {
	case MAX2831_VARIANT:
		max2831_reg_write(&drv->drv.max2831, r, v);
		break;

	case MAX2837_VARIANT:
		max2837_reg_write(&drv->drv.max2837, r, v);
		break;

	case MAX2839_VARIANT:
		max2839_reg_write(&drv->drv.max2839, r, v);
		break;
	}
}

/* Write all dirty registers via SPI from memory. Mark all clean. Some
 * operations require registers to be written in a certain order. Use
 * provided routines for those operations. */
void max283x_regs_commit(max283x_driver_t* const drv)
{
	switch (drv->type) {
	case MAX2831_VARIANT:
		max2831_regs_commit(&drv->drv.max2831);
		break;

	case MAX2837_VARIANT:
		max2837_regs_commit(&drv->drv.max2837);
		break;

	case MAX2839_VARIANT:
		max2839_regs_commit(&drv->drv.max2839);
		break;
	}
}

void max283x_set_mode(max283x_driver_t* const drv, const max283x_mode_t new_mode)
{
	switch (drv->type) {
	case MAX2831_VARIANT:
		max2831_set_mode(&drv->drv.max2831, (max2831_mode_t) new_mode);
		break;

	case MAX2837_VARIANT:
		max2837_set_mode(&drv->drv.max2837, (max2837_mode_t) new_mode);
		break;

	case MAX2839_VARIANT:
		max2839_set_mode(&drv->drv.max2839, (max2839_mode_t) new_mode);
		break;
	}
}

max283x_mode_t max283x_mode(max283x_driver_t* const drv)
{
	switch (drv->type) {
	case MAX2831_VARIANT:
		return (max283x_mode_t) max2831_mode(&drv->drv.max2831);
		break;

	case MAX2837_VARIANT:
		return (max283x_mode_t) max2837_mode(&drv->drv.max2837);
		break;

	case MAX2839_VARIANT:
		return (max283x_mode_t) max2839_mode(&drv->drv.max2839);
		break;
	}

	return 0;
}

//max283x_mode_t max283x_mode(max283x_driver_t* const drv);
//void max283x_set_mode(max283x_driver_t* const drv, const max283x_mode_t new_mode);

/* Turn on/off all chip functions. Does not control oscillator and CLKOUT */
void max283x_start(max283x_driver_t* const drv)
{
	switch (drv->type) {
	case MAX2831_VARIANT:
		max2831_start(&drv->drv.max2831);
		break;

	case MAX2837_VARIANT:
		max2837_start(&drv->drv.max2837);
		break;

	case MAX2839_VARIANT:
		max2839_start(&drv->drv.max2839);
		break;
	}
}

void max283x_stop(max283x_driver_t* const drv)
{
	switch (drv->type) {
	case MAX2831_VARIANT:
		max2831_stop(&drv->drv.max2831);
		break;

	case MAX2837_VARIANT:
		max2837_stop(&drv->drv.max2837);
		break;

	case MAX2839_VARIANT:
		max2839_stop(&drv->drv.max2839);
		break;
	}
}

/* Set frequency in Hz. Frequency setting is a multi-step function
 * where order of register writes matters. */
void max283x_set_frequency(max283x_driver_t* const drv, uint32_t freq)
{
	switch (drv->type) {
	case MAX2831_VARIANT:
		max2831_set_frequency(&drv->drv.max2831, freq);
		break;

	case MAX2837_VARIANT:
		max2837_set_frequency(&drv->drv.max2837, freq);
		break;

	case MAX2839_VARIANT:
		max2839_set_frequency(&drv->drv.max2839, freq);
		break;
	}
}

uint32_t max283x_set_lpf_bandwidth(
	max283x_driver_t* const drv,
	const uint32_t bandwidth_hz)
{
	switch (drv->type) {
	case MAX2831_VARIANT:
		return max2831_set_lpf_bandwidth(&drv->drv.max2831, bandwidth_hz);
		break;

	case MAX2837_VARIANT:
		return max2837_set_lpf_bandwidth(&drv->drv.max2837, bandwidth_hz);
		break;

	case MAX2839_VARIANT:
		return max2839_set_lpf_bandwidth(&drv->drv.max2839, bandwidth_hz);
		break;
	}

	return 0;
}

bool max283x_set_lna_gain(max283x_driver_t* const drv, const uint32_t gain_db)
{
	switch (drv->type) {
	case MAX2831_VARIANT:
		return max2831_set_lna_gain(&drv->drv.max2831, gain_db);
		break;

	case MAX2837_VARIANT:
		return max2837_set_lna_gain(&drv->drv.max2837, gain_db);
		break;

	case MAX2839_VARIANT:
		return max2839_set_lna_gain(&drv->drv.max2839, gain_db);
		break;
	}

	return false;
}

bool max283x_set_vga_gain(max283x_driver_t* const drv, const uint32_t gain_db)
{
	switch (drv->type) {
	case MAX2831_VARIANT:
		return max2831_set_vga_gain(&drv->drv.max2831, gain_db);
		break;

	case MAX2837_VARIANT:
		return max2837_set_vga_gain(&drv->drv.max2837, gain_db);
		break;

	case MAX2839_VARIANT:
		return max2839_set_vga_gain(&drv->drv.max2839, gain_db);
		break;
	}

	return false;
}

bool max283x_set_txvga_gain(max283x_driver_t* const drv, const uint32_t gain_db)
{
	switch (drv->type) {
	case MAX2831_VARIANT:
		return max2831_set_txvga_gain(&drv->drv.max2831, gain_db);
		break;

	case MAX2837_VARIANT:
		return max2837_set_txvga_gain(&drv->drv.max2837, gain_db);
		break;

	case MAX2839_VARIANT:
		return max2839_set_txvga_gain(&drv->drv.max2839, gain_db);
		break;
	}

	return false;
}

void max283x_tx(max283x_driver_t* const drv)
{
	switch (drv->type) {
	case MAX2831_VARIANT:
		max2831_tx(&drv->drv.max2831);
		break;

	case MAX2837_VARIANT:
		max2837_tx(&drv->drv.max2837);
		break;

	case MAX2839_VARIANT:
		max2839_tx(&drv->drv.max2839);
		break;
	}
}

void max283x_rx(max283x_driver_t* const drv)
{
	switch (drv->type) {
	case MAX2831_VARIANT:
		max2831_rx(&drv->drv.max2831);
		break;

	case MAX2837_VARIANT:
		max2837_rx(&drv->drv.max2837);
		break;

	case MAX2839_VARIANT:
		max2839_rx(&drv->drv.max2839);
		break;
	}
}

void max283x_tx_calibration(max283x_driver_t* const drv)
{
	switch (drv->type) {
	case MAX2831_VARIANT:
		max2831_tx_calibration(&drv->drv.max2831);
		break;

	case MAX2837_VARIANT:
		// unsupported - uses max283x_set_mode
		break;

	case MAX2839_VARIANT:
		// unsupported - uses max283x_set_mode
		break;
	}
}

void max283x_rx_calibration(max283x_driver_t* const drv)
{
	switch (drv->type) {
	case MAX2831_VARIANT:
		max2831_rx_calibration(&drv->drv.max2831);
		break;

	case MAX2837_VARIANT:
		// unsupported - uses max283x_set_mode
		break;

	case MAX2839_VARIANT:
		// unsupported - uses max283x_set_mode
		break;
	}
}
