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

cmake_minimum_required(VERSION 3.10.0)
set(CMAKE_TOOLCHAIN_FILE toolchain-arm-cortex-m.cmake)



if(NOT DEFINED FLASH_MB_SIZE)
    set(FLASH_MB_SIZE 1) # Default to 1MB if not provided. For PortaRf this should be 2MB passed to cmake with -DFLASH_MB_SIZE=2
endif()
message("Configuring for FLASH_MB_SIZE=${FLASH_MB_SIZE}MB")
#This is the flash memory size we build. This can be less, so the FW size will be less, so can be flashed with older utils.
if(NOT DEFINED FLASH_MB_LIMIT_SIZE)
	set(FLASH_MB_LIMIT_SIZE 1) #TODO: should be ${FLASH_MB_SIZE} remove once we got a stable build for all devices we support with bigger than 1 mb flash
endif()
message("Configuring for FLASH_MB_LIMIT_SIZE=${FLASH_MB_LIMIT_SIZE}MB")

if(FLASH_MB_SIZE LESS FLASH_MB_LIMIT_SIZE)
    message(FATAL_ERROR "Error: The supported flash size is lower than the generated flash file. Build halted.")
endif()

if(NOT DEFINED FLASH_BYTES_SIZE)
    math(EXPR FLASH_BYTES_SIZE "${FLASH_MB_SIZE} * 1024 * 1024")
endif()
if(NOT DEFINED FLASH_BYTES_LIMIT_SIZE)
    math(EXPR FLASH_BYTES_LIMIT_SIZE "${FLASH_MB_LIMIT_SIZE} * 1024 * 1024")
endif()

#generate h file
configure_file(../flashsize.h.in ${CMAKE_CURRENT_SOURCE_DIR}/../flashsize.h)


project (hackrf_firmware_all C)

add_subdirectory(blinky)
add_subdirectory(hackrf_usb)
