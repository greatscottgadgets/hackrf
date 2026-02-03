# Copyright 2009 Uwe Hermann <uwe@hermann-uwe.de>
# Copyright 2010 Piotr Esden-Tempski <piotr@esden.net>
# Copyright 2012 Michael Ossmann <mike@ossmann.com>
# Copyright 2012 Benjamin Vernoux <titanmkd@gmail.com>
# Copyright 2012 Jared Boone <jared@sharebrained.com>
# Copyright 2016 Dominic Spill <dominicgs@gmail.com>
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

# derived primarily from Makefiles in libopencm3

enable_language(C CXX ASM)

SET(PATH_HACKRF_FIRMWARE ${CMAKE_CURRENT_LIST_DIR})
if(NOT DEFINED SGPIO_DEBUG)
	SET(PATH_HACKRF_CPLD_XSVF ${PATH_HACKRF_FIRMWARE}/cpld/sgpio_if/default.xsvf)
else()
	SET(PATH_HACKRF_CPLD_XSVF ${PATH_HACKRF_FIRMWARE}/cpld/sgpio_debug/default.xsvf)
endif()
SET(VAR_USED ${SGPIO_DEBUG})
SET(PATH_HACKRF ${PATH_HACKRF_FIRMWARE}/..)
SET(PATH_HACKRF_FIRMWARE_COMMON ${PATH_HACKRF_FIRMWARE}/common)
SET(LIBOPENCM3 ${PATH_HACKRF_FIRMWARE}/libopencm3)
SET(PATH_DFU_PY ${PATH_HACKRF_FIRMWARE}/dfu.py)
SET(PATH_CPLD_BITSTREAM_TOOL ${PATH_HACKRF_FIRMWARE}/tools/cpld_bitstream.py)
set(PATH_HACKRF_CPLD_DATA_C ${CMAKE_CURRENT_BINARY_DIR}/hackrf_cpld_data.c)
SET(PATH_PRALINE_FPGA_BIN ${PATH_HACKRF_FIRMWARE}/fpga/build/praline_fpga.bin)
SET(PATH_PRALINE_FPGA_OBJ ${CMAKE_CURRENT_BINARY_DIR}/fpga.o)

include(${PATH_HACKRF_FIRMWARE}/dfu-util.cmake)

include(ExternalProject)
ExternalProject_Add(libopencm3_${PROJECT_NAME}
	SOURCE_DIR "${LIBOPENCM3}"
	BUILD_IN_SOURCE true
	DOWNLOAD_COMMAND ""
	CONFIGURE_COMMAND ""
	INSTALL_COMMAND ""
)

if (NOT DEFINED VERSION)
	execute_process(
		COMMAND git log -n 1 --format=%h
		WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
		RESULT_VARIABLE GIT_VERSION_FOUND
		ERROR_QUIET
		OUTPUT_VARIABLE GIT_VERSION
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)
	if (GIT_VERSION_FOUND)
		set(VERSION "2026.01.3+")
	else (GIT_VERSION_FOUND)
		set(VERSION "git-${GIT_VERSION}")
	endif (GIT_VERSION_FOUND)
endif()

if(NOT DEFINED BOARD)
	set(BOARD HACKRF_ONE)
endif()

if(BOARD STREQUAL "HACKRF_ONE" OR BOARD STREQUAL "PRALINE")
	set(MCU_PARTNO LPC4320)
else()
	set(MCU_PARTNO LPC4330)
endif()

if(NOT DEFINED SRC_M0)
	set(SRC_M0 "${PATH_HACKRF_FIRMWARE_COMMON}/m0_sleep.c")
endif()

SET(HACKRF_OPTS "-D${BOARD} -DLPC43XX -D${MCU_PARTNO} -DTX_ENABLE -D'VERSION_STRING=\"${VERSION}\"'")

SET(LDSCRIPT_M4 "-T${PATH_HACKRF_FIRMWARE_COMMON}/${MCU_PARTNO}_M4_memory.ld -Tlibopencm3_lpc43xx_rom_to_ram.ld -T${PATH_HACKRF_FIRMWARE_COMMON}/LPC43xx_M4_M0_image_from_text.ld -T${PATH_HACKRF_FIRMWARE_COMMON}/LPC43xx_M4_memory_rom_only.ld")

SET(LDSCRIPT_M4_RAM "-T${PATH_HACKRF_FIRMWARE_COMMON}/${MCU_PARTNO}_M4_memory.ld -Tlibopencm3_lpc43xx.ld -T${PATH_HACKRF_FIRMWARE_COMMON}/LPC43xx_M4_M0_image_from_text.ld")

SET(LDSCRIPT_M0 "-T${PATH_HACKRF_FIRMWARE_COMMON}/LPC43xx_M0_memory.ld -Tlibopencm3_lpc43xx_m0.ld")

SET(CFLAGS_COMMON "-Os -g3 -Wall -Wextra ${HACKRF_OPTS} -fno-common -MD")
SET(LDFLAGS_COMMON "-nostartfiles -Wl,--gc-sections")

if(V STREQUAL "1")
	SET(LDFLAGS_COMMON "${LDFLAGS_COMMON} -Wl,--print-gc-sections")
endif()

SET(CPUFLAGS_M0 "-mthumb -mcpu=cortex-m0 -mfloat-abi=soft")
SET(CFLAGS_M0 "-std=gnu99 ${CFLAGS_COMMON} ${CPUFLAGS_M0} -DLPC43XX_M0")
SET(CXXFLAGS_M0 "-std=gnu++0x ${CFLAGS_COMMON} ${CPUFLAGS_M0} -DLPC43XX_M0")
SET(LDFLAGS_M0 "${LDFLAGS_COMMON} ${CPUFLAGS_M0} ${LDSCRIPT_M0} -Xlinker -Map=m0.map")

SET(CPUFLAGS_M4 "-mthumb -mcpu=cortex-m4 -mfloat-abi=hard -mfpu=fpv4-sp-d16")
SET(CFLAGS_M4 "-std=gnu99 ${CFLAGS_COMMON} ${CPUFLAGS_M4} -DLPC43XX_M4")
SET(CXXFLAGS_M4 "-std=gnu++0x ${CFLAGS_COMMON} ${CPUFLAGS_M4} -DLPC43XX_M4")
SET(LDFLAGS_M4 "${LDFLAGS_COMMON} ${CPUFLAGS_M4} ${LDSCRIPT_M4} -Xlinker -Map=m4.map")

SET(CFLAGS_M4_RAM "-std=gnu99 ${CFLAGS_COMMON} ${CPUFLAGS_M4} -DLPC43XX_M4")
SET(LDFLAGS_M4_RAM "${LDFLAGS_COMMON} ${CPUFLAGS_M4} ${LDSCRIPT_M4_RAM} -Xlinker -Map=m4.map")

set(BUILD_SHARED_LIBS OFF)

include_directories("${LIBOPENCM3}/include/")
include_directories("${PATH_HACKRF_FIRMWARE_COMMON}")

macro(DeclareTarget project_name variant_suffix cflags ldflags)
	# Generate M0 bin from elf
	add_custom_command(
		OUTPUT ${project_name}${variant_suffix}_m0.bin
		DEPENDS ${PROJECT_NAME}_m0.elf
		COMMAND ${CMAKE_OBJCOPY} -Obinary ${PROJECT_NAME}_m0.elf ${project_name}${variant_suffix}_m0.bin
	)

	# Generate asm source that includes the M0 bin
	add_custom_command(
		OUTPUT ${project_name}${variant_suffix}_m0_bin.s
		DEPENDS
			${project_name}${variant_suffix}_m0.bin
			${PATH_HACKRF_FIRMWARE_COMMON}/m0_bin.s.cmake
			${PATH_HACKRF_FIRMWARE_COMMON}/configure_file.cmake
		COMMAND
			${CMAKE_COMMAND}
			-DSRC=${PATH_HACKRF_FIRMWARE_COMMON}/m0_bin.s.cmake
			-DDEST=${project_name}${variant_suffix}_m0_bin.s
			-DBIN=${CMAKE_CURRENT_BINARY_DIR}/${project_name}${variant_suffix}_m0.bin
			-P ${PATH_HACKRF_FIRMWARE_COMMON}/configure_file.cmake
		# `configure_file` only updates the output if the input changed, but it can't recognise when the bin changes.
		# So, force an update with `touch`
		COMMAND ${CMAKE_COMMAND} -E touch ${project_name}${variant_suffix}_m0_bin.s
	)

	add_library(${project_name}${variant_suffix}_objects OBJECT ${SRC_M4} ${project_name}${variant_suffix}_m0_bin.s)
	set_target_properties(${project_name}${variant_suffix}_objects PROPERTIES COMPILE_FLAGS "${cflags}")
	add_executable(${project_name}${variant_suffix}.elf $<TARGET_OBJECTS:${project_name}${variant_suffix}_objects> ${OBJ_M4})
	add_dependencies(${project_name}${variant_suffix}.elf libopencm3_${project_name})

	target_link_libraries(
		${project_name}${variant_suffix}.elf
		c
		nosys
		opencm3_lpc43xx
		m
	)

	set_target_properties(${project_name}${variant_suffix}.elf PROPERTIES LINK_FLAGS "${ldflags}")

	add_custom_target(
		${project_name}${variant_suffix}.bin ALL
		DEPENDS ${project_name}${variant_suffix}.elf
		COMMAND ${CMAKE_OBJCOPY} -Obinary ${project_name}${variant_suffix}.elf ${project_name}${variant_suffix}.bin
	)
endmacro()

macro(DeclareTargets)
	SET(SRC_M4
		${SRC_M4}
		${PATH_HACKRF_FIRMWARE_COMMON}/hackrf_core.c
		${PATH_HACKRF_FIRMWARE_COMMON}/sgpio.c
		${PATH_HACKRF_FIRMWARE_COMMON}/rf_path.c
		${PATH_HACKRF_FIRMWARE_COMMON}/si5351c.c
		${PATH_HACKRF_FIRMWARE_COMMON}/max5864.c
		${PATH_HACKRF_FIRMWARE_COMMON}/max5864_target.c
		${PATH_HACKRF_FIRMWARE_COMMON}/mixer.c
		${PATH_HACKRF_FIRMWARE_COMMON}/i2c_bus.c
		${PATH_HACKRF_FIRMWARE_COMMON}/i2c_lpc.c
		${PATH_HACKRF_FIRMWARE_COMMON}/w25q80bv.c
		${PATH_HACKRF_FIRMWARE_COMMON}/w25q80bv_target.c
		${PATH_HACKRF_FIRMWARE_COMMON}/spi_bus.c
		${PATH_HACKRF_FIRMWARE_COMMON}/spi_ssp.c
		${PATH_HACKRF_FIRMWARE_COMMON}/gpio_lpc.c
		${PATH_HACKRF_FIRMWARE_COMMON}/hackrf_ui.c
		${PATH_HACKRF_FIRMWARE_COMMON}/platform_detect.c
		${PATH_HACKRF_FIRMWARE_COMMON}/firmware_info.c
		${PATH_HACKRF_FIRMWARE_COMMON}/clkin.c
		${PATH_HACKRF_FIRMWARE_COMMON}/gpdma.c
		${PATH_HACKRF_FIRMWARE_COMMON}/user_config.c
		${PATH_HACKRF_FIRMWARE_COMMON}/radio.c
		${PATH_HACKRF_FIRMWARE_COMMON}/selftest.c
		${PATH_HACKRF_FIRMWARE_COMMON}/m0_state.c
		${PATH_HACKRF_FIRMWARE_COMMON}/adc.c
		)

	if(BOARD STREQUAL "RAD1O")
		SET(SRC_M4
			${SRC_M4}
			${PATH_HACKRF_FIRMWARE_COMMON}/max2871.c
			${PATH_HACKRF_FIRMWARE_COMMON}/max2871_regs.c
		)
	else()
		SET(SRC_M4
			${SRC_M4}
			${PATH_HACKRF_FIRMWARE_COMMON}/rffc5071.c
			${PATH_HACKRF_FIRMWARE_COMMON}/rffc5071_spi.c
		)
	endif()

	if(BOARD STREQUAL "PRALINE")
		SET(SRC_M4
			${SRC_M4}
			${PATH_HACKRF_FIRMWARE_COMMON}/fpga.c
			${PATH_HACKRF_FIRMWARE_COMMON}/ice40_spi.c
			${PATH_HACKRF_FIRMWARE_COMMON}/max2831.c
			${PATH_HACKRF_FIRMWARE_COMMON}/max2831_target.c
		)
	else()
		SET(SRC_M4
			${SRC_M4}
			${PATH_HACKRF_FIRMWARE_COMMON}/max283x.c
			${PATH_HACKRF_FIRMWARE_COMMON}/max2837.c
			${PATH_HACKRF_FIRMWARE_COMMON}/max2837_target.c
			${PATH_HACKRF_FIRMWARE_COMMON}/max2839.c
			${PATH_HACKRF_FIRMWARE_COMMON}/max2839_target.c
		)
	endif()

	link_directories(
		"${PATH_HACKRF_FIRMWARE_COMMON}"
		"${LIBOPENCM3}/lib"
		"${LIBOPENCM3}/lib/lpc43xx"
		"${CMAKE_INSTALL_PREFIX}/lib/armv7e-m/fpu"
	)

	add_executable(${PROJECT_NAME}_m0.elf ${SRC_M0})
	add_dependencies(${PROJECT_NAME}_m0.elf libopencm3_${PROJECT_NAME})

	target_link_libraries(
		${PROJECT_NAME}_m0.elf
		c
		nosys
		opencm3_lpc43xx_m0
	)

	set_target_properties(${PROJECT_NAME}_m0.elf PROPERTIES COMPILE_FLAGS "${CFLAGS_M0}")
	set_target_properties(${PROJECT_NAME}_m0.elf PROPERTIES LINK_FLAGS "${LDFLAGS_M0}")

	DeclareTarget("${PROJECT_NAME}" "" "${CFLAGS_M4}" "${LDFLAGS_M4}")	
	DeclareTarget("${PROJECT_NAME}" "_ram" "${CFLAGS_M4_RAM} -DRAM_MODE" "${LDFLAGS_M4_RAM}")	
	DeclareTarget("${PROJECT_NAME}" "_dfu" "${CFLAGS_M4_RAM} -DDFU_MODE" "${LDFLAGS_M4_RAM}")	

	add_custom_target(
		${PROJECT_NAME}.dfu ${DFU_ALL}
		DEPENDS ${PROJECT_NAME}_dfu.bin
		COMMAND rm -f _header.bin
		COMMAND python3 ${PATH_DFU_PY} ${PROJECT_NAME}
		COMMAND cat _header.bin ${PROJECT_NAME}_dfu.bin > ${PROJECT_NAME}.dfu
		COMMAND dfu-suffix --vid=0x1fc9 --pid=0x000c --did=0x0 -a ${PROJECT_NAME}.dfu
		COMMAND rm -f _header.bin
	)

	# Program / flash targets
	add_custom_target(
		${PROJECT_NAME}-flash
		DEPENDS ${PROJECT_NAME}.bin
		COMMAND hackrf_spiflash -Rw ${PROJECT_NAME}.bin
	)

	add_custom_target(
		${PROJECT_NAME}-program
		DEPENDS ${PROJECT_NAME}.dfu
		COMMAND dfu-util --device 1fc9:000c --alt 0 --download ${PROJECT_NAME}.dfu
	)
endmacro()
