# Copyright 2012 Jared Boone
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

# Based heavily upon the libftdi cmake setup.

project(libhackrf C)
set(MAJOR_VERSION 0)
set(MINOR_VERSION 1)
set(PACKAGE libhackrf)
set(VERSION_STRING ${MAJOR_VERSION}.${MINOR_VERSION})
set(VERSION ${VERSION_STRING})
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}")
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/Modules)

cmake_minimum_required(VERSION 2.8)

if (WIN32)
  set(THREADS_USE_PTHREADS_WIN32 true)
  find_package(Threads REQUIRED)
  include_directories(${THREADS_PTHREADS_INCLUDE_DIR})
endif()

find_package(USB1 REQUIRED)
include_directories(${LIBUSB_INCLUDE_DIR})

if(CMAKE_COMPILER_IS_GNUCC)
add_definitions(-Wall)
endif()

add_subdirectory(src)
add_subdirectory(examples)

