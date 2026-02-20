/*
 * Copyright 2022 Great Scott Gadgets <info@greatscottgadgets.com>
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

#include "firmware_info.h"
#include "platform_detect.h"
#include "gpio_lpc.h"
#include "hackrf_core.h"

#include <libopencm3/lpc43xx/scu.h>
#include <libopencm3/lpc43xx/adc.h>

#ifdef JAWBREAKER
	#define SUPPORTED_PLATFORM PLATFORM_JAWBREAKER
#elif HACKRF_ONE
	#define SUPPORTED_PLATFORM (PLATFORM_HACKRF1_OG | PLATFORM_HACKRF1_R9)
#elif RAD1O
	#define SUPPORTED_PLATFORM PLATFORM_RAD1O
#elif PRALINE
	#define SUPPORTED_PLATFORM PLATFORM_PRALINE
#elif HACKRF_ALL
	#define SUPPORTED_PLATFORM \
		(PLATFORM_PRALINE | PLATFORM_HACKRF1_OG | PLATFORM_HACKRF1_R9)
#else
	#define SUPPORTED_PLATFORM 0
#endif

#ifdef DFU_MODE
	#define DFU_MODE_VALUE 1
#else
	#define DFU_MODE_VALUE 0
#endif

__attribute__((section(".firmware_info"))) const struct firmware_info_t firmware_info = {
	.magic = "HACKRFFW",
	.struct_version = 1,
	.dfu_mode = DFU_MODE_VALUE,
	.supported_platform = SUPPORTED_PLATFORM,
	.version_string = VERSION_STRING,
};
