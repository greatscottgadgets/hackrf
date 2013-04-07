# Hey Emacs, this is a -*- makefile -*-
#
# Copyright 2013 Benjamin Vernoux <titanmkd@gmail.com>
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

BINARY = blinky_rom_to_ram

SRC_DIR = blinky

SRC = blinky.c \
	../common/hackrf_core.c \
	../common/si5351c.c \
	../common/max2837.c

LDSCRIPT = ../common/LPC4330_M4_rom_to_ram.ld

%.o: ../$(SRC_DIR)/%.c  Makefile
	@printf "  CC      $(subst $(shell pwd)/,,$(@))\n"
	$(Q)$(CC) $(CFLAGS) -o $@ -c $<

include ../common/Makefile_inc.mk
