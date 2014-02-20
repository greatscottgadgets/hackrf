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

# Based heavily upon the libftdi cmake setup.

cmake_minimum_required(VERSION 2.8)
project(hackrf-tools C)
set(MAJOR_VERSION 0)
set(MINOR_VERSION 3)
set(PACKAGE hackrf-tools)
set(VERSION_STRING ${MAJOR_VERSION}.${MINOR_VERSION})
set(VERSION ${VERSION_STRING})
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/../cmake/modules)

if(MSVC)
include_directories(getopt)
add_definitions(/D _CRT_SECURE_NO_WARNINGS)
else()
add_definitions(-Wall)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu90")
endif()

if(NOT libhackrf_SOURCE_DIR)
find_package(LIBHACKRF REQUIRED)
include_directories(${LIBHACKRF_INCLUDE_DIR})
else()
include_directories(${libhackrf_SOURCE_DIR}/src)
endif()

add_subdirectory(src)

########################################################################
# Create uninstall target
########################################################################

if(NOT hackrf_all_SOURCE_DIR)
configure_file(
    ${PROJECT_SOURCE_DIR}/../cmake/cmake_uninstall.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
@ONLY)


add_custom_target(uninstall
    ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
)
endif()

########################################################################
# Install udev rules
########################################################################
option(INSTALL_UDEV_RULES "Install udev rules for HackRF" OFF)
if (INSTALL_UDEV_RULES)
    install (
        FILES 52-hackrf.rules
        DESTINATION "/etc/udev/rules.d"
        COMPONENT "udev"
        )
else (INSTALL_UDEV_RULES)
    message (STATUS "Udev rules not being installed, install them with -DINSTALL_UDEV_RULES=ON")
endif (INSTALL_UDEV_RULES)
