# Copyright 2012 Jared Boone <jared@sharebrained.com>
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

# Top directory CMake project for HackRF firmware

cmake_minimum_required(VERSION 3.1.3)
set(CMAKE_TOOLCHAIN_FILE toolchain-arm-cortex-m.cmake)

project (hackrf_firmware_all C)

SET(PATH_HACKRF_FIRMWARE ${CMAKE_CURRENT_LIST_DIR})
SET(PATH_HACKRF ${PATH_HACKRF_FIRMWARE}/..)
SET(PATH_HACKRF_FIRMWARE_COMMON ${PATH_HACKRF_FIRMWARE}/common)
SET(LIBOPENCM3 ${PATH_HACKRF_FIRMWARE}/libopencm3)
SET(PATH_DFU_PY ${PATH_HACKRF_FIRMWARE}/dfu.py)

include(ExternalProject)
ExternalProject_Add(libopencm3
	SOURCE_DIR "${LIBOPENCM3}"
	BUILD_IN_SOURCE true
	DOWNLOAD_COMMAND ""
	CONFIGURE_COMMAND ""
	INSTALL_COMMAND ""
)

add_subdirectory(blinky)
add_subdirectory(hackrf_usb)
