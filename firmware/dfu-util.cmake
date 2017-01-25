#
# Copyright 2015 Dominic Spill <dominicgs@gmail.com>
#
# This file is part of GreatFET.
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

execute_process(
	COMMAND dfu-suffix -V
	WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
	RESULT_VARIABLE DFU_NOT_FOUND
	ERROR_QUIET
	OUTPUT_VARIABLE DFU_VERSION_STRING
	OUTPUT_STRIP_TRAILING_WHITESPACE
)

set(DFU_ALL "")
if(NOT DFU_NOT_FOUND)
	string(REGEX REPLACE ".*([0-9]+)\\.[0-9]+.*" "\\1" DFU_VERSION_MAJOR "${DFU_VERSION_STRING}")
	string(REGEX REPLACE ".*[0-9]+\\.([0-9])+.*" "\\1" DFU_VERSION_MINOR "${DFU_VERSION_STRING}")
	MESSAGE( STATUS "DFU utils version: " ${DFU_VERSION_MAJOR} "." ${DFU_VERSION_MINOR})
	execute_process(
	        COMMAND dfu-prefix -V
		WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
		RESULT_VARIABLE DFU_PREFIX_NOT_FOUND
		ERROR_QUIET
		OUTPUT_VARIABLE DFU_PREFIX_VERSION_STRING
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)
	if(DFU_PREFIX_NOT_FOUND)
		set(DFU_COMMAND dfu-suffix --vid=0x1fc9 --pid=0x000c --did=0x0 -s 0 -a _tmp.dfu)
	else(DFU_PREFIX_NOT_FOUND)
		set(DFU_COMMAND dfu-suffix --vid=0x1fc9 --pid=0x000c --did=0x0 -a _tmp.dfu && dfu-prefix -s 0 -a _tmp.dfu)
	endif(DFU_PREFIX_NOT_FOUND)
    set(DFU_ALL "ALL")
else(NOT DFU_NOT_FOUND)
	MESSAGE(STATUS "dfu-suffix not found: not building DFU file")
endif(NOT DFU_NOT_FOUND)



    