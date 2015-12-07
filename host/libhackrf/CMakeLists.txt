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
project(libhackrf C)
set(MAJOR_VERSION 0)
set(MINOR_VERSION 4)
set(PACKAGE libhackrf)
set(VERSION_STRING ${MAJOR_VERSION}.${MINOR_VERSION})
set(VERSION ${VERSION_STRING})
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/../cmake/modules)

if(MSVC)
	set(THREADS_USE_PTHREADS_WIN32 true)
else()
	add_definitions(-Wall)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu90")

	INCLUDE(TestBigEndian)
	TEST_BIG_ENDIAN(BIGENDIAN)
	if(${BIGENDIAN})
		 add_definitions(-DHACKRF_BIG_ENDIAN)
	endif(${BIGENDIAN})
endif()
find_package(USB1 REQUIRED)
find_package(Threads REQUIRED)

include_directories(${LIBUSB_INCLUDE_DIR} ${THREADS_PTHREADS_INCLUDE_DIR})

add_subdirectory(src)

########################################################################
# Create Pkg Config File
########################################################################
FOREACH(inc ${LIBUSB_INCLUDE_DIR})
    LIST(APPEND HACKRF_PC_CFLAGS "-I${inc}")
ENDFOREACH(inc)

# use space-separation format for the pc file
STRING(REPLACE ";" " " HACKRF_PC_CFLAGS "${HACKRF_PC_CFLAGS}")
STRING(REPLACE ";" " " HACKRF_PC_LIBS "${HACKRF_PC_LIBS}")

# unset these vars to avoid hard-coded paths to cross environment
IF(CMAKE_CROSSCOMPILING)
    UNSET(HACKRF_PC_CFLAGS)
    UNSET(HACKRF_PC_LIBS)
ENDIF(CMAKE_CROSSCOMPILING)

set(prefix ${CMAKE_INSTALL_PREFIX})
set(exec_prefix \${prefix})
set(libdir \${exec_prefix}/lib${LIB_SUFFIX})
set(includedir \${prefix}/include)
set(libpkgdata lib${LIB_SUFFIX})

if(CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
    set(libpkgdata "libdata")
endif(CMAKE_SYSTEM_NAME MATCHES "FreeBSD")

CONFIGURE_FILE(
    ${CMAKE_CURRENT_SOURCE_DIR}/libhackrf.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/libhackrf.pc
@ONLY)

INSTALL(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/libhackrf.pc
    DESTINATION ${libpkgdata}/pkgconfig
)

########################################################################
# Create Pkg Config File
########################################################################

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    SET(SYSTEM_IS_LINUX TRUE)
    SET(UDEV_OPTION_DEFAULT ON)
else()
    SET(SYSTEM_IS_LINUX FALSE)
    SET(UDEV_OPTION_DEFAULT OFF)
endif()

option(INSTALL_UDEV_RULES
    "Install udev rules for the HackRF"
    ${UDEV_OPTION_DEFAULT}
)

set(UDEV_RULES_PATH
    "/etc/udev/rules.d"
    CACHE STRING
    "Target directory for udev rule installation. Ensure you have permissions to write to this directory."
)

if(SYSTEM_IS_LINUX)
    if(INSTALL_UDEV_RULES)
        if(NOT DEFINED UDEV_RULES_GROUP)
            foreach(group usb plugdev)
                execute_process(COMMAND "getent" group "${group}"
                                RESULT_VARIABLE _GETENT_RESULT
                                OUTPUT_QUIET
                                ERROR_QUIET)
                if(NOT _GETENT_RESULT)
                    message(STATUS "Setting udev rule group to - ${group}")
                    set(UDEV_RULES_GROUP ${group})
                    break()
                endif(NOT _GETENT_RESULT)
            endforeach(group)
        endif(NOT DEFINED UDEV_RULES_GROUP)
        if(DEFINED UDEV_RULES_GROUP)
            set(HACKRF_GROUP "${UDEV_RULES_GROUP}"
                CACHE STRING "Group to associate HackRF devices with in udev rules")
            configure_file(${CMAKE_CURRENT_SOURCE_DIR}/53-hackrf.rules.in
                        ${CMAKE_CURRENT_BINARY_DIR}/53-hackrf.rules
                        @ONLY
                        )
            message(STATUS  "HackRF udev rules will be installed to '${UDEV_RULES_PATH}' upon running 'make install'")
            install(FILES ${CMAKE_CURRENT_BINARY_DIR}/53-hackrf.rules
                    DESTINATION ${UDEV_RULES_PATH}
                    COMPONENT "udev_rules")
        else(UDEV_RULES_GROUP)
            message(STATUS "HackRF udev rules will not be installed because no suitable group was found")
            message(STATUS "A group can be specified with -DUDEV_RULES_GROUP=<group>")
        endif(DEFINED UDEV_RULES_GROUP)
    else(INSTALL_UDEV_RULES)
        message(STATUS
                "HackRF udev rules will not be installed because INSTALL_UDEV_RULES=OFF"
               )
    endif(INSTALL_UDEV_RULES)
else(SYSTEM_IS_LINUX)
    if(INSTALL_UDEV_RULES)
        message(STATUS "udev rules not supported on this platform. Hide this message via -DINSTALL_UDEV_RULES=Off")
    endif(INSTALL_UDEV_RULES)
endif(SYSTEM_IS_LINUX)

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
