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

option(CHECK_INCLUDES
	   "Check firmware sources for unused includes and transitive dependencies. (Requires iwyu)" OFF)

cmake_minimum_required(VERSION 3.12.0)
set(CMAKE_TOOLCHAIN_FILE toolchain-arm-cortex-m.cmake)

project (hackrf_firmware_all C)

if(CHECK_INCLUDES)
	include(include-what-you-use.cmake)
endif()

add_subdirectory(blinky)
add_subdirectory(hackrf_usb)
