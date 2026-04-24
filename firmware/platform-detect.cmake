# Copyright 2026 Great Scott Gadgets <info@greatscottgadgets.com>
#
# This file is part of HackRF.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street,
# Boston, MA 02110-1301, USA.
#

cmake_minimum_required(VERSION 3.12.0)

add_compile_definitions(
	BOARD_ID_JELLYBEAN=0
	BOARD_ID_JAWBREAKER=1
	BOARD_ID_HACKRF1_OG=2 # HackRF One prior to r9
	BOARD_ID_RAD1O=3
	BOARD_ID_HACKRF1_R9=4
	BOARD_ID_PRALINE=5
	BOARD_ID_UNRECOGNIZED=0xFE # tried detection but did not recognize board
	BOARD_ID_UNDETECTED=0xFF   # detection not yet attempted
)

if(BOARD STREQUAL "UNIVERSAL")
	add_compile_definitions(
		"IS_UNIVERSAL=1"
		"IS_PRALINE=(detected_platform() == BOARD_ID_PRALINE)"
		"IS_NOT_PRALINE=(!IS_PRALINE)"
		"IS_HACKRF_ONE=( \
			detected_platform() == BOARD_ID_HACKRF1_OG || \
			detected_platform() == BOARD_ID_HACKRF1_R9 \
		)"
		"IS_H1_R9=(detected_platform() == BOARD_ID_HACKRF1_R9)"
		"IS_NOT_H1_R9=(!IS_H1_R9)"
		"IS_NOT_RAD1O=1"
		"IS_NOT_JAWBREAKER=1"
		"IS_H1_OR_PRALINE=1"
		"IS_H1_OR_RAD1O=IS_HACKRF_ONE"
		"IS_H1_OR_JAWBREAK=IS_HACKRF_ONE"
		"IS_FOUR_LEDS=IS_PRALINE"
		"IS_EXPANSION_COMPATIBLE=1"
	)
elseif(BOARD STREQUAL "HACKRF_ONE")
	add_compile_definitions(
		"IS_NOT_PRALINE=1"
		"IS_HACKRF_ONE=1"
		"IS_H1_R9=(detected_platform() == BOARD_ID_HACKRF1_R9)"
		"IS_NOT_H1_R9=(!IS_H1_R9)"
		"IS_NOT_RAD1O=1"
		"IS_NOT_JAWBREAKER=1"
		"IS_H1_OR_PRALINE=1"
		"IS_H1_OR_RAD1O=1"
		"IS_H1_OR_JAWBREAK=1"
		"IS_EXPANSION_COMPATIBLE=1"
	)
elseif(BOARD STREQUAL "PRALINE")
	add_compile_definitions(
		"IS_PRALINE=1"
		"IS_NOT_HACKRF_ONE=1"
		"IS_NOT_H1_R9=1"
		"IS_NOT_RAD1O=1"
		"IS_NOT_JAWBREAKER=1"
		"IS_H1_OR_PRALINE=1"
		"IS_FOUR_LEDS=1"
		"IS_EXPANSION_COMPATIBLE=1"
	)
elseif(BOARD STREQUAL "RAD1O")
	add_compile_definitions(
		"IS_NOT_PRALINE=1"
		"IS_NOT_HACKRF_ONE=1"
		"IS_NOT_H1_R9=1"
		"IS_RAD1O=1"
		"IS_NOT_JAWBREAKER=1"
		"IS_H1_OR_RAD1O=1"
		"IS_FOUR_LEDS=1"
	)
elseif(BOARD STREQUAL "JAWBREAKER")
	add_compile_definitions(
		"IS_NOT_PRALINE=1"
		"IS_NOT_HACKRF_ONE=1"
		"IS_NOT_H1_R9=1"
		"IS_NOT_RAD1O=1"
		"IS_JAWBREAKER=1"
		"IS_H1_OR_JAWBREAKER=1"
	)
else()
	message(FATAL_ERROR "No recognized platform defined: ${BOARD}")
endif()
