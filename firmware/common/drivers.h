/*
 * Copyright 2026 Great Scott Gadgets <info@greatscottgadgets.com>
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

#include "cpld_jtag.h"
#include "platform_detect.h" // IWYU pragma: keep
#include "sgpio.h"
#if defined(IS_PRALINE)
	#include "fpga.h"
	#include "ice40_spi.h"
	#include "spi_ssp.h"
#endif

#if defined(IS_PRALINE)
extern ssp_config_t ssp_config_ice40_fpga;
extern ice40_spi_driver_t ice40;
extern fpga_driver_t fpga;
#endif
extern sgpio_config_t sgpio_config;
extern jtag_gpio_t jtag_gpio_cpld;
extern jtag_t jtag_cpld;

#if defined(IS_PRALINE)
void ssp1_set_mode_ice40(void);
#endif

#ifdef __cplusplus
}
#endif
