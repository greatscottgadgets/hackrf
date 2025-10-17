# Copyright 2012 Jared Boone
# Copyright 2013 Benjamin Vernoux
#
# This file is part of HackRF.
#
# This program is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation; either version 2, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along with
# this program; see the file COPYING.  If not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Boston, MA 02110-1301, USA.

cmake_minimum_required(VERSION 3.10.0)
project(hackrf-tools LANGUAGES C)
include(GNUInstallDirs)
set(PACKAGE hackrf-tools)

include(${PROJECT_SOURCE_DIR}/../cmake/set_release.cmake)
add_definitions(-DTOOL_RELEASE="${RELEASE}")
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/../cmake/modules)

if(MSVC)
  include_directories(getopt)
  add_definitions(/D _CRT_SECURE_NO_WARNINGS)
else()
  add_definitions(-Wall)
endif()

if(NOT libhackrf_SOURCE_DIR)
  if(LIBHACKRF_INCLUDE_DIR
     AND LIBHACKRF_LIBRARIES
     AND NOT TARGET HackRF::hackrf)
    add_library(HackRF::hackrf INTERFACE IMPORTED)
    set_target_properties(
      HackRF::hackrf
      PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${LIBHACKRF_INCLUDE_DIR}"
                 INTERFACE_LINK_LIBRARIES "${LIBHACKRF_LIBRARIES}")
  else()
    find_package(HackRF REQUIRED)
  endif()
endif()

add_subdirectory(src)

########################################################################
# Create uninstall target
########################################################################

if(NOT HackRF_SOURCE_DIR)
  configure_file(${PROJECT_SOURCE_DIR}/../cmake/cmake_uninstall.cmake.in
                 ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake @ONLY)

  add_custom_target(
    uninstall
    COMMENT "Provide hackrf-tools uninstall target"
    COMMAND ${CMAKE_COMMAND} -P
            ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
endif()
