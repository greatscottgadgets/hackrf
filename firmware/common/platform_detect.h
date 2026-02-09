/*
 * Copyright 2022-2022 Great Scott Gadgets <info@greatscottgadgets.com>
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

#ifndef __PLATFORM_DETECT_H__
#define __PLATFORM_DETECT_H__

#include <stdint.h>

#define BOARD_REV_GSG (0x80)

#define PLATFORM_JAWBREAKER (1 << 0)
#define PLATFORM_HACKRF1_OG (1 << 1)
#define PLATFORM_RAD1O      (1 << 2)
#define PLATFORM_HACKRF1_R9 (1 << 3)
#define PLATFORM_PRALINE    (1 << 4)

typedef enum {
	BOARD_ID_JELLYBEAN = 0,
	BOARD_ID_JAWBREAKER = 1,
	BOARD_ID_HACKRF1_OG = 2, /* HackRF One prior to r9 */
	BOARD_ID_RAD1O = 3,
	BOARD_ID_HACKRF1_R9 = 4,
	BOARD_ID_PRALINE = 5,
	BOARD_ID_UNRECOGNIZED = 0xFE, /* tried detection but did not recognize board */
	BOARD_ID_UNDETECTED = 0xFF,   /* detection not yet attempted */
} board_id_t;

typedef enum {
	BOARD_REV_HACKRF1_OLD = 0,
	BOARD_REV_HACKRF1_R6 = 1,
	BOARD_REV_HACKRF1_R7 = 2,
	BOARD_REV_HACKRF1_R8 = 3,
	BOARD_REV_HACKRF1_R9 = 4,
	BOARD_REV_HACKRF1_R10 = 5,
	BOARD_REV_PRALINE_R0_1 = 6,
	BOARD_REV_PRALINE_R0_2 = 7,
	BOARD_REV_PRALINE_R0_3 = 8,
	BOARD_REV_PRALINE_R1_0 = 9,
	BOARD_REV_PRALINE_R1_1 = 10,
	BOARD_REV_PRALINE_R1_2 = 11,
	BOARD_REV_GSG_HACKRF1_R6 = 0x81,
	BOARD_REV_GSG_HACKRF1_R7 = 0x82,
	BOARD_REV_GSG_HACKRF1_R8 = 0x83,
	BOARD_REV_GSG_HACKRF1_R9 = 0x84,
	BOARD_REV_GSG_HACKRF1_R10 = 0x85,
	BOARD_REV_HACKRF1_ADC_BASE = 32,
	// Board revisions having the analog voltage detection but not mapped to a specific hardware revision fit in this range
	BOARD_REV_HACKRF1_ADC_MAX = 63,
	BOARD_REV_GSG_PRALINE_R0_1 = 0x86,
	BOARD_REV_GSG_PRALINE_R0_2 = 0x87,
	BOARD_REV_GSG_PRALINE_R0_3 = 0x88,
	BOARD_REV_GSG_PRALINE_R1_0 = 0x89,
	BOARD_REV_GSG_PRALINE_R1_1 = 0x8a,
	BOARD_REV_GSG_PRALINE_R1_2 = 0x8b,
	BOARD_REV_UNRECOGNIZED =
		0xFE,                /* tried detection but did not recognize revision */
	BOARD_REV_UNDETECTED = 0xFF, /* detection not yet attempted */
} board_rev_t;

void detect_hardware_platform(void);
board_id_t detected_platform(void);
board_rev_t detected_revision(void);
uint32_t supported_platform(void);
void finalize_detect_hardware_platform(void);

#endif //__PLATFORM_DETECT_H__
