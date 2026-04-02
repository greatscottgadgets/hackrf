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

cmake_minimum_required(VERSION 3.10.0)

find_program(IWYU_COMMAND NAMES include-what-you-use)
if(NOT IWYU_COMMAND)
	message(FATAL_ERROR "Could not find the include-what-you-use executable.")
endif()

# include-what-you-use arguments:
#
#   --no_fwd_decls  - Disable forward declaration suggestions.
#   --mapping_file  - Specify mapping files for excluding suggestions.
#
list(APPEND IWYU_COMMAND
			-Xiwyu --no_fwd_decls
			-Xiwyu --mapping_file=${CMAKE_SOURCE_DIR}/libopencm3.imp)

set(CMAKE_C_INCLUDE_WHAT_YOU_USE ${IWYU_COMMAND})
set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE ${IWYU_COMMAND})
