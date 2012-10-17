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

option(EXAMPLES "Build example programs" ON)

IF( EXAMPLES )
   add_executable(hackrf_max2837 hackrf_max2837.c)
   add_executable(hackrf_si5351c hackrf_si5351c.c)
   add_executable(hackrf_transfer hackrf_transfer.c)
   
   target_link_libraries(hackrf_max2837 hackrf)
   target_link_libraries(hackrf_si5351c hackrf)
   target_link_libraries(hackrf_transfer hackrf)
   
   include_directories(BEFORE ${CMAKE_SOURCE_DIR}/src)
endif(EXAMPLES)
