# Copyright 2012 Jared Boone
# Copyright 2013 Benjamin Vernoux
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

cmake_minimum_required(VERSION 3.8)
# standard policies for CMake 3.8
cmake_policy(VERSION 3.8)

# Based heavily upon the libftdi cmake setup.
project(hackrf-tools LANGUAGES C)

# Run common init code
# -----------------------------------------------------
if(NOT HACKRF_HOST_INCLUDED)
	include(../cmake/hackrf-host.cmake)
endif()

# configure release definition
set(PACKAGE hackrf-tools)
include(set_release)
add_definitions(-DTOOL_RELEASE="${RELEASE}")


if(MSVC)
	include_directories(getopt)
endif()

# Find tools dependencies
# -----------------------------------------------------

# fftw3
find_package(FFTW COMPONENTS SinglePrecision REQUIRED)

# libm
find_package(CMath REQUIRED)

# if we are not building libhackrf, we need to find a preexisting one in the system.
if(TARGET hackrf)
	# building libhackrf at the same time, use it.
	set(HACKRF_LIBRARY_TARGET hackrf)
else()
	# standalone tools build
	find_package(LIBHACKRF REQUIRED)
	set(HACKRF_LIBRARY_TARGET libhackrf::libhackrf)
endif()

# Build the code
# -----------------------------------------------------
add_subdirectory(src)
