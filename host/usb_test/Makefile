# Hey Emacs, this is a -*- makefile -*-
#
# Copyright 2010 Michael Ossmann <mike@ossmann.com>
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

# derived primarily from Makefiles in Project Ubertooth

CC      ?= gcc
AR      ?= ar
INSTALL ?= /usr/bin/install
LDCONFIG ?= /sbin/ldconfig

OPTFLAGS   = -O2
ANGRYFLAGS = -Wall #-Wextra -pedantic
CFLAGS    += $(OPTFLAGS) $(ANGRYFLAGS)

LIB_DIR ?= /usr/lib
INCLUDE_DIR ?= /usr/include
INSTALL_DIR ?= /usr/bin

OS = $(shell uname)
ifeq ($(OS), FreeBSD)
	LIBUSB = usb
	CFLAGS += -DFREEBSD
else
	LIBUSB = usb-1.0
endif

LDFLAGS += -l$(LIBUSB)

TOOLS = usb_test

ifeq ($(OS), Darwin)
	CFLAGS += -I/opt/local/include/
	LDFLAGS += -L/opt/local/lib/ -rpath,/opt/local/lib
else
	#
endif

TOOLS_SOURCE = $(TOOLS:%=%.c)

all: $(TOOLS)

$(TOOLS): $(TOOLS_SOURCE)
		$(CC) $(CFLAGS) -o $@ $@.c $(LDFLAGS)

install: $(TOOLS)
	$(INSTALL) -m 0755 $(TOOLS) $(INSTALL_DIR)

cygwin-install: $(LIB_FILE) $(STATIC_LIB_FILE)
	$(INSTALL) -m 0755 $(TOOLS) $(INSTALL_DIR)

osx-install: $(LIB_FILE) $(TOOLS)
	$(INSTALL) -m 0755 $(TOOLS) $(INSTALL_DIR)

clean:
	rm -f $(TOOLS)

.PHONY: all clean install cygwin-install osx-install