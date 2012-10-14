# Hey Emacs, this is a -*- makefile -*-
#
# Copyright 2009 Uwe Hermann <uwe@hermann-uwe.de>
# Copyright 2010 Piotr Esden-Tempski <piotr@esden.net>
# Copyright 2012 Michael Ossmann <mike@ossmann.com>
# Copyright 2012 Benjamin Vernoux <titanmkd@gmail.com>
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

# derived primarily from Makefiles in libopencm3

BOARD ?= JELLYBEAN

HACKRF_OPTS = -D$(BOARD)

# comment to disable RF transmission
HACKRF_OPTS += -DTX_ENABLE

LDSCRIPT ?= ../common/LPC4330_M4.ld

LIBOPENCM3 ?= /usr/local/arm-none-eabi

PREFIX ?= arm-none-eabi
#PREFIX ?= arm-elf
CC = $(PREFIX)-gcc
LD = $(PREFIX)-gcc
OBJCOPY = $(PREFIX)-objcopy
OBJDUMP = $(PREFIX)-objdump
GDB = $(PREFIX)-gdb
TOOLCHAIN_DIR := $(shell dirname `which $(CC)`)/../$(PREFIX)

CFLAGS += -std=c99 -Os -g3 -Wall -Wextra -I$(LIBOPENCM3)/include -I../common \
		-fno-common -mcpu=cortex-m4 -mthumb -MD \
		-mfloat-abi=hard -mfpu=fpv4-sp-d16 \
		$(HACKRF_OPTS)
#LDSCRIPT ?= $(BINARY).ld
LDFLAGS += -L$(TOOLCHAIN_DIR)/lib/armv7e-m/fpu \
    -L../common \
		-L$(LIBOPENCM3)/lib -L$(LIBOPENCM3)/lib/lpc43xx \
		-T$(LDSCRIPT) -nostartfiles \
		-Wl,--gc-sections -Xlinker -Map=$(BINARY).map
OBJ = $(SRC:.c=.o)

# Be silent per default, but 'make V=1' will show all compiler calls.
ifneq ($(V),1)
Q := @
NULL := 2>/dev/null
else
LDFLAGS += -Wl,--print-gc-sections
endif

.SUFFIXES: .elf .bin .hex .srec .list .images
.SECONDEXPANSION:
.SECONDARY:

all: images

images: $(BINARY).images
flash: $(BINARY).flash

program: $(BINARY).dfu
	$(Q)dfu-util --device 1fc9:000c --alt 0 --download $(BINARY).dfu

%.images: %.bin %.hex %.srec %.list
	@#echo "*** $* images generated ***"

%.dfu: %.bin
	$(Q)rm -f _tmp.dfu _header.bin
	$(Q)cp $(*).bin _tmp.dfu
	$(Q)dfu-suffix --vid=0x1fc9 --pid=0x000c --did=0x0 -s 0 -a _tmp.dfu
	$(Q)python -c "import os.path; import struct; print('0000000: da ff ' + ' '.join(map(lambda s: '%02x' % ord(s), struct.pack('<H', os.path.getsize('$(*).bin') / 512 + 1))) + ' ff ff ff ff')" | xxd -g1 -r > _header.bin
	$(Q)cat _header.bin _tmp.dfu >$(*).dfu
	$(Q)rm -f _tmp.dfu _header.bin

%.bin: %.elf
	@#printf "  OBJCOPY $(*).bin\n"
	$(Q)$(OBJCOPY) -Obinary -j .text -j .ARM.exidx $(*).elf $(*).bin

%.hex: %.elf
	@#printf "  OBJCOPY $(*).hex\n"
	$(Q)$(OBJCOPY) -Oihex $(*).elf $(*).hex

%.srec: %.elf
	@#printf "  OBJCOPY $(*).srec\n"
	$(Q)$(OBJCOPY) -Osrec $(*).elf $(*).srec

%.list: %.elf
	@#printf "  OBJDUMP $(*).list\n"
	$(Q)$(OBJDUMP) -S $(*).elf > $(*).list

%.elf: $(OBJ) $(LDSCRIPT)
	@#printf "  LD      $(subst $(shell pwd)/,,$(@))\n"
	$(Q)$(LD) $(LDFLAGS) -o $(*).elf $(OBJ) -lopencm3_lpc43xx

%.o: %.c Makefile
	@#printf "  CC      $(subst $(shell pwd)/,,$(@))\n"
	$(Q)$(CC) $(CFLAGS) -o $@ -c $<

clean:
	$(Q)rm -f *.o
	$(Q)rm -f *.d
	$(Q)rm -f *.elf
	$(Q)rm -f *.bin
	$(Q)rm -f *.dfu
	$(Q)rm -f _tmp.dfu _header.bin
	$(Q)rm -f *.hex
	$(Q)rm -f *.srec
	$(Q)rm -f *.list
	$(Q)rm -f *.map
	$(Q)rm -f *.lst
	$(Q)rm -f ../common/*.o
	$(Q)rm -f ../common/*.d
	$(Q)rm -f ../common/*.lst

.PHONY: images clean

-include $(OBJ:.o=.d)
