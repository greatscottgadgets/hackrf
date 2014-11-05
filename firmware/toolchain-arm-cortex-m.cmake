# Copyright 2014 Jared Boone <jared@sharebrained.com>
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

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_VERSION 1)
set(CMAKE_SYSTEM_PROCESSOR arm)

include(CMakeForceCompiler)

CMAKE_FORCE_C_COMPILER(arm-none-eabi-gcc GNU)
CMAKE_FORCE_CXX_COMPILER(arm-none-eabi-g++ GNU)

execute_process(
  COMMAND ${CMAKE_C_COMPILER} -print-file-name=libc.a
  OUTPUT_VARIABLE CMAKE_INSTALL_PREFIX
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
get_filename_component(CMAKE_INSTALL_PREFIX
  "${CMAKE_INSTALL_PREFIX}" PATH
)
get_filename_component(CMAKE_INSTALL_PREFIX
  "${CMAKE_INSTALL_PREFIX}/.." REALPATH
)
set(CMAKE_INSTALL_PREFIX  ${CMAKE_INSTALL_PREFIX} CACHE FILEPATH
    "Install path prefix, prepended onto install directories.")

message(STATUS "Cross-compiling with the gcc-arm-embedded toolchain")
message(STATUS "Toolchain prefix: ${CMAKE_INSTALL_PREFIX}")

set(CMAKE_ASM_COMPILER ${CMAKE_C_COMPILER})
#set(CMAKE_LD ${CMAKE_INSTALL_PREFIX}/bin/ld CACHE INTERNAL "ld tool")
set(CMAKE_OBJCOPY ${CMAKE_INSTALL_PREFIX}/bin/objcopy CACHE INTERNAL "objcopy tool")

set(CMAKE_FIND_ROOT_PATH ${CMAKE_INSTALL_PREFIX})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
