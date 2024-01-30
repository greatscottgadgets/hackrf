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

find_program(CCACHE "ccache")
if(CCACHE)
    set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE})
    set(ENV{CCACHE_SLOPPINESS} pch_defines,time_macros)
endif(CCACHE)

add_subdirectory(blinky)
add_subdirectory(hackrf_usb)
