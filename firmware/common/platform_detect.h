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

#pragma once

#include <stdint.h>

#define BOARD_REV_GSG (0x80)

#define PLATFORM_JAWBREAKER (1 << 0)
#define PLATFORM_HACKRF1_OG (1 << 1)
#define PLATFORM_RAD1O      (1 << 2)
#define PLATFORM_HACKRF1_R9 (1 << 3)
#define PLATFORM_PRALINE    (1 << 4)

/* clang-format off */

/* Helper macros for platform-specific code. */
#if defined(UNIVERSAL)
	#define IF_PRALINE(...) \
		if (detected_platform() == BOARD_ID_PRALINE) { __VA_ARGS__; }
	#define IF_NOT_PRALINE(...) \
		if (detected_platform() != BOARD_ID_PRALINE) { __VA_ARGS__; }
	#define IF_HACKRF_ONE(...) \
		switch (detected_platform()) { \
		case BOARD_ID_HACKRF1_OG: \
		case BOARD_ID_HACKRF1_R9: \
			{ __VA_ARGS__; } \
			break; \
		default: \
			break; \
		}
	#define IF_NOT_HACKRF_ONE(...) \
		switch (detected_platform()) { \
		case BOARD_ID_HACKRF1_OG: \
		case BOARD_ID_HACKRF1_R9: \
			break; \
		default: \
			{ __VA_ARGS__; } \
			break; \
		}
	#define IF_H1_R9(...) \
		if (detected_platform() == BOARD_ID_HACKRF1_R9) { __VA_ARGS__; }
	#define IF_NOT_H1_R9(...) \
		if (detected_platform() != BOARD_ID_HACKRF1_R9) { __VA_ARGS__; }
	#define IF_RAD1O(...)
	#define IF_NOT_RAD1O(...) { __VA_ARGS__; }
	#define IF_JAWBREAKER(...)
	#define IF_NOT_JAWBREAKER(...) { __VA_ARGS__; }
	#define IF_H1_OR_PRALINE(...) { __VA_ARGS__; }
	#define IF_H1_OR_RAD1O(...) IF_HACKRF_ONE(__VA_ARGS__)
	#define IF_H1_OR_JAWBREAKER(...) IF_HACKRF_ONE(__VA_ARGS__)
	#define IF_FOUR_LEDS(...) IF_PRALINE(__VA_ARGS__)
	#define IF_EXPANSION_COMPATIBLE(...) { __VA_ARGS__; }
#elif defined(HACKRF_ONE)
	#define IF_PRALINE(...)
	#define IF_NOT_PRALINE(...) { __VA_ARGS__; }
	#define IF_HACKRF_ONE(...) { __VA_ARGS__; }
	#define IF_NOT_HACKRF_ONE(...)
	#define IF_H1_R9(...) \
		if (detected_platform() == BOARD_ID_HACKRF1_R9) { __VA_ARGS__; }
	#define IF_NOT_H1_R9(...) \
		if (detected_platform() != BOARD_ID_HACKRF1_R9) { __VA_ARGS__; }
	#define IF_RAD1O(...)
	#define IF_NOT_RAD1O(...) { __VA_ARGS__; }
	#define IF_JAWBREAKER(...)
	#define IF_NOT_JAWBREAKER(...) { __VA_ARGS__; }
	#define IF_H1_OR_PRALINE(...) { __VA_ARGS__; }
	#define IF_H1_OR_RAD1O(...) { __VA_ARGS__; }
	#define IF_H1_OR_JAWBREAKER(...) { __VA_ARGS__; }
	#define IF_FOUR_LEDS(...)
	#define IF_EXPANSION_COMPATIBLE(...) { __VA_ARGS__; }
#elif defined(PRALINE)
	#define IF_PRALINE(...) { __VA_ARGS__; }
	#define IF_NOT_PRALINE(...)
	#define IF_HACKRF_ONE(...)
	#define IF_NOT_HACKRF_ONE(...) { __VA_ARGS__; }
	#define IF_H1_R9(...)
	#define IF_NOT_H1_R9(...) { __VA_ARGS__; }
	#define IF_RAD1O(...)
	#define IF_NOT_RAD1O(...) { __VA_ARGS__; }
	#define IF_JAWBREAKER(...)
	#define IF_NOT_JAWBREAKER(...) { __VA_ARGS__; }
	#define IF_H1_OR_PRALINE(...) { __VA_ARGS__; }
	#define IF_H1_OR_RAD1O(...)
	#define IF_H1_OR_JAWBREAKER(...)
	#define IF_FOUR_LEDS(...)
	#define IF_EXPANSION_COMPATIBLE(...) { __VA_ARGS__; }
#elif defined(RAD1O)
	#define IF_PRALINE(...)
	#define IF_NOT_PRALINE(...) { __VA_ARGS__; }
	#define IF_HACKRF_ONE(...)
	#define IF_NOT_HACKRF_ONE(...) { __VA_ARGS__; }
	#define IF_H1_R9(...)
	#define IF_NOT_H1_R9(...) { __VA_ARGS__; }
	#define IF_RAD1O(...) { __VA_ARGS__; }
	#define IF_NOT_RAD1O(...)
	#define IF_JAWBREAKER(...)
	#define IF_NOT_JAWBREAKER(...) { __VA_ARGS__; }
	#define IF_H1_OR_PRALINE(...)
	#define IF_H1_OR_RAD1O(...) { __VA_ARGS__; }
	#define IF_H1_OR_JAWBREAKER(...)
	#define IF_FOUR_LEDS(...) { __VA_ARGS__; }
	#define IF_EXPANSION_COMPATIBLE(...)
#elif defined(JAWBREAKER)
	#define IF_PRALINE(...)
	#define IF_NOT_PRALINE(...) { __VA_ARGS__; }
	#define IF_HACKRF_ONE(...)
	#define IF_NOT_HACKRF_ONE(...) { __VA_ARGS__; }
	#define IF_H1_R9(...)
	#define IF_NOT_H1_R9(...) { __VA_ARGS__; }
	#define IF_RAD1O(...)
	#define IF_NOT_RAD1O(...) { __VA_ARGS__; }
	#define IF_JAWBREAKER(...) { __VA_ARGS__; }
	#define IF_NOT_JAWBREAKER(...)
	#define IF_H1_OR_PRALINE(...)
	#define IF_H1_OR_RAD1O(...)
	#define IF_H1_OR_JAWBREAKER(...) { __VA_ARGS__; }
	#define IF_FOUR_LEDS(...)
	#define IF_EXPANSION_COMPATIBLE(...)
#else
	#error "No recognised platform defined"
#endif

/* clang-format on */

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
