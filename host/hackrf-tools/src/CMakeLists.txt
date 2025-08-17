# Copyright 2012 Jared Boone
# Copyright 2013 Benjamin Vernoux
# Copyright 2025 A. Maitland Bottoms <bottoms@debian.org>
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
#

option(ENABLE_HACKRF_SWEEP
       "Build and Install hackrf_sweep tool (Requires FFTW3f)" ON)
find_package(FFTW3f)

set(TOOLS
    hackrf_transfer
    hackrf_spiflash
    hackrf_cpldjtag
    hackrf_info
    hackrf_debug
    hackrf_clock
    hackrf_operacake
    hackrf_biast)

if(TARGET HackRF::hackrf)
  list(APPEND TOOLS_LINK_LIBS HackRF::hackrf)
else()
  list(APPEND TOOLS_LINK_LIBS HackRF::hackrf_static)
endif()

if(MSVC)
  add_library(libgetopt_static STATIC ../getopt/getopt.c)
  list(APPEND TOOLS_LINK_LIBS libgetopt_static)
endif()

include(CheckLibraryExists)
check_library_exists(m log10 "" LIBM)
if(LIBM)
  list(APPEND TOOLS_LINK_LIBS m)
endif()

foreach(tool ${TOOLS})
  add_executable(${tool} ${tool}.c)
  target_compile_features(${tool} PRIVATE c_std_90)
  target_link_libraries(${tool} ${TOOLS_LINK_LIBS})
  install(TARGETS ${tool} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endforeach(tool)
if(PC_FFTW3f_FOUND AND ENABLE_HACKRF_SWEEP)
  add_executable(hackrf_sweep hackrf_sweep.c)
  target_compile_features(hackrf_sweep PRIVATE c_std_90)
  target_link_libraries(hackrf_sweep fftw3f::fftw3f ${TOOLS_LINK_LIBS})
  install(TARGETS hackrf_sweep RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
else()
  message(
    STATUS
      "Not building hackrf_sweep tool. (Install FFTW, set ENABLE_HACKRF_SWEEP)"
  )
endif()

if(WIN32)
  install(
    DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIGURATION>/"
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    FILES_MATCHING
    PATTERN "fftw*.dll")
endif()
