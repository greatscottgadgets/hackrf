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

set(INSTALL_DEFAULT_BINDIR "bin" CACHE STRING "Appended to CMAKE_INSTALL_PREFIX")

if(MSVC)
add_library(libgetopt_static STATIC
    ../getopt/getopt.c
)
endif()

add_executable(hackrf_max2837 hackrf_max2837.c)
install(TARGETS hackrf_max2837 RUNTIME DESTINATION ${INSTALL_DEFAULT_BINDIR})

add_executable(hackrf_si5351c hackrf_si5351c.c)
install(TARGETS hackrf_si5351c RUNTIME DESTINATION ${INSTALL_DEFAULT_BINDIR})

add_executable(hackrf_transfer hackrf_transfer.c)
install(TARGETS hackrf_transfer RUNTIME DESTINATION ${INSTALL_DEFAULT_BINDIR})

add_executable(hackrf_rffc5071 hackrf_rffc5071.c)
install(TARGETS hackrf_rffc5071 RUNTIME DESTINATION ${INSTALL_DEFAULT_BINDIR})

add_executable(hackrf_spiflash hackrf_spiflash.c)
install(TARGETS hackrf_spiflash RUNTIME DESTINATION ${INSTALL_DEFAULT_BINDIR})

add_executable(hackrf_cpldjtag hackrf_cpldjtag.c)
install(TARGETS hackrf_cpldjtag RUNTIME DESTINATION ${INSTALL_DEFAULT_BINDIR})

add_executable(hackrf_info hackrf_info.c)
install(TARGETS hackrf_info RUNTIME DESTINATION ${INSTALL_DEFAULT_BINDIR})

include_directories(${LIBHACKRF_INCLUDE_DIR})

target_link_libraries(hackrf_max2837 ${LIBHACKRF_LIBRARIES})
target_link_libraries(hackrf_si5351c ${LIBHACKRF_LIBRARIES})
target_link_libraries(hackrf_transfer ${LIBHACKRF_LIBRARIES})
target_link_libraries(hackrf_rffc5071 ${LIBHACKRF_LIBRARIES})
target_link_libraries(hackrf_spiflash ${LIBHACKRF_LIBRARIES})
target_link_libraries(hackrf_cpldjtag ${LIBHACKRF_LIBRARIES})
target_link_libraries(hackrf_info ${LIBHACKRF_LIBRARIES})

if(MSVC)
target_link_libraries(hackrf_max2837  libgetopt_static)
target_link_libraries(hackrf_si5351c  libgetopt_static)
target_link_libraries(hackrf_transfer  libgetopt_static)
target_link_libraries(hackrf_rffc5071  libgetopt_static)
target_link_libraries(hackrf_spiflash  libgetopt_static)
target_link_libraries(hackrf_cpldjtag  libgetopt_static)
target_link_libraries(hackrf_info  libgetopt_static)
endif()
