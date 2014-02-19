# Hey Emacs, this is a -*- makefile -*-
#
# Copyright 2012 Jared Boone <jared@sharebrained.com>
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

ifeq ($(RUN_FROM),RAM)
	BINARY = hackrf_usb_ram
else
	BINARY = hackrf_usb_rom_to_ram
endif

SRC_M4_C = hackrf_usb.c \
	../common/rf_path.c \
	../common/tuning.c \
	../common/streaming.c \
	sgpio_isr.c \
	usb_bulk_buffer.c \
	../common/usb.c \
	../common/usb_request.c \
	../common/usb_standard_request.c \
	usb_descriptor.c \
	usb_device.c \
	usb_endpoint.c \
	usb_api_board_info.c \
	usb_api_cpld.c \
	usb_api_register.c \
	usb_api_spiflash.c \
	usb_api_transceiver.c \
	../common/usb_queue.c \
	../common/fault_handler.c \
	../common/hackrf_core.c \
	../common/sgpio.c \
	../common/si5351c.c \
	../common/max2837.c \
	../common/max5864.c \
	../common/rffc5071.c \
	../common/w25q80bv.c \
	../common/cpld_jtag.c \
	../common/xapp058/lenval.c \
	../common/xapp058/micro.c \
	../common/xapp058/ports.c \
	../common/rom_iap.c

include ../common/Makefile_inc.mk
