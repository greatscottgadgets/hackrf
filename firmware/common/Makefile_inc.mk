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

BOARD ?= HACKRF_ONE
RUN_FROM ?= SPIFI

ifeq ($(BOARD),HACKRF_ONE)
	MCU_PARTNO=LPC4320
else
	MCU_PARTNO=LPC4330
endif

HACKRF_OPTS = -D$(BOARD) -DLPC43XX -D$(MCU_PARTNO)

# comment to disable RF transmission
HACKRF_OPTS += -DTX_ENABLE

# automatic git version when working out of git
VERSION_STRING ?= -D'VERSION_STRING="git-$(shell git log -n 1 --format=%h)"'
HACKRF_OPTS += $(VERSION_STRING)

PATH_HACKRF ?= ../..

PATH_HACKRF_FIRMWARE = $(PATH_HACKRF)/firmware
PATH_HACKRF_FIRMWARE_COMMON = $(PATH_HACKRF_FIRMWARE)/common

LIBOPENCM3 ?= $(PATH_HACKRF_FIRMWARE)/libopencm3

VPATH += $(PATH_HACKRF_FIRMWARE_COMMON)/xapp058
VPATH += $(PATH_HACKRF_FIRMWARE_COMMON)

SRC_M4_C ?= $(SRC)
SRC_M0_C ?= $(PATH_HACKRF_FIRMWARE_COMMON)/m0_sleep.c

BUILD_DIR = build
OBJDIR_M4 = $(BUILD_DIR)/m4
OBJDIR_M0 = $(BUILD_DIR)/m0

OBJ_M4_C = $(patsubst %.c, $(OBJDIR_M4)/%.o, $(notdir $(SRC_M4_C)))
OBJ_M4_S = $(patsubst %.s, $(OBJDIR_M4)/%.o, $(notdir $(SRC_M4_S)))

OBJ_M0_C = $(patsubst %.c, $(OBJDIR_M0)/%.o, $(notdir $(SRC_M0_C)))
OBJ_M0_S = $(patsubst %.s, $(OBJDIR_M0)/%.o, $(notdir $(SRC_M0_S)))

LDSCRIPT_M4 += -T$(PATH_HACKRF_FIRMWARE_COMMON)/$(MCU_PARTNO)_M4_memory.ld
ifeq ($(RUN_FROM),RAM)
	LDSCRIPT_M4 += -Tlibopencm3_lpc43xx.ld
else
	LDSCRIPT_M4 += -Tlibopencm3_lpc43xx_rom_to_ram.ld
endif
LDSCRIPT_M4 += -T$(PATH_HACKRF_FIRMWARE_COMMON)/LPC43xx_M4_M0_image_from_text.ld

LDSCRIPT_M0 += -T$(PATH_HACKRF_FIRMWARE_COMMON)/LPC43xx_M0_memory.ld
LDSCRIPT_M0 += -Tlibopencm3_lpc43xx_m0.ld

PREFIX ?= arm-none-eabi
CC = $(PREFIX)-gcc
LD = $(PREFIX)-gcc
OBJCOPY = $(PREFIX)-objcopy
OBJDUMP = $(PREFIX)-objdump
GDB = $(PREFIX)-gdb
TOOLCHAIN_DIR := $(shell dirname `which $(CC)`)/../$(PREFIX)

CFLAGS_COMMON += -std=gnu99 -Os -g3 -Wall -Wextra -I$(LIBOPENCM3)/include -I$(PATH_HACKRF_FIRMWARE_COMMON) \
		$(HACKRF_OPTS) -fno-common -mthumb -MD
LDFLAGS_COMMON += -mthumb \
		-L$(PATH_HACKRF_FIRMWARE_COMMON) \
		-L$(LIBOPENCM3)/lib -L$(LIBOPENCM3)/lib/lpc43xx \
		-nostartfiles \
		-Wl,--gc-sections \
		-lc -lnosys
CFLAGS_M0 += -mcpu=cortex-m0 -DLPC43XX_M0
LDFLAGS_M0 += -mcpu=cortex-m0 -DLPC43XX_M0
LDFLAGS_M0 += $(LDSCRIPT_M0)
LDFLAGS_M0 += --specs=nano.specs
LDFLAGS_M0 += -Xlinker -Map=$(OBJDIR_M0)/m0.map
LDFLAGS_M0 += -lopencm3_lpc43xx_m0

CFLAGS_M4 += -mcpu=cortex-m4 -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DLPC43XX_M4
LDFLAGS_M4 += -mcpu=cortex-m4 -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DLPC43XX_M4
LDFLAGS_M4 += -L$(TOOLCHAIN_DIR)/lib/armv7e-m/fpu
LDFLAGS_M4 += $(LDSCRIPT_M4)
LDFLAGS_M4 += -Xlinker -Map=$(OBJDIR_M4)/m4.map
LDFLAGS_M4 += -lopencm3_lpc43xx -lm

# Be silent per default, but 'make V=1' will show all compiler calls.
ifneq ($(V),1)
Q := @
NULL := 2>/dev/null
else
LDFLAGS_COMMON += -Wl,--print-gc-sections
endif

.SUFFIXES: .elf .bin .hex .srec .list .images
.SECONDEXPANSION:
.SECONDARY:

all: images

images: $(BINARY).images
flash: $(BINARY).flash

program: $(BINARY).dfu
	$(Q)dfu-util --device 1fc9:000c --alt 0 --download $(BINARY).dfu

$(BINARY).images: $(BINARY).bin $(BINARY).hex $(BINARY).srec $(BINARY).list
	@#echo "*** $* images generated ***"

$(BINARY).dfu: $(BINARY).bin
	$(Q)rm -f _tmp.dfu _header.bin
	$(Q)cp $(BINARY).bin _tmp.dfu
	$(Q)dfu-suffix --vid=0x1fc9 --pid=0x000c --did=0x0 -s 0 -a _tmp.dfu
	$(Q)python -c "import os.path; import struct; print('0000000: da ff ' + ' '.join(map(lambda s: '%02x' % ord(s), struct.pack('<H', os.path.getsize('$(BINARY).bin') / 512 + 1))) + ' ff ff ff ff')" | xxd -g1 -r > _header.bin
	$(Q)cat _header.bin _tmp.dfu >$(BINARY).dfu
	$(Q)rm -f _tmp.dfu _header.bin

$(BINARY).bin: $(BINARY).elf
	@#printf "  OBJCOPY $(*).bin\n"
	$(Q)$(OBJCOPY) -Obinary $(BINARY).elf $(BINARY).bin

$(OBJDIR_M0)/m0.bin: $(OBJDIR_M0)/m0.elf
	@#printf "  OBJCOPY $(*).bin\n"
	$(Q)$(OBJCOPY) -Obinary $(OBJDIR_M0)/m0.elf $(OBJDIR_M0)/m0.bin

#$(OBJDIR_M0)/m0.o: $(OBJDIR_M0)/m0.bin
#	$(Q)$(OBJCOPY) -I binary -B arm -O elf32-littlearm $(OBJDIR_M0)/m0.bin $(OBJDIR_M0)/m0.o

$(BINARY).hex: $(BINARY).elf
	@#printf "  OBJCOPY $(BINARY).hex\n"
	$(Q)$(OBJCOPY) -Oihex $(BINARY).elf $(BINARY).hex

$(BINARY).srec: $(BINARY).elf
	@#printf "  OBJCOPY $(BINARY).srec\n"
	$(Q)$(OBJCOPY) -Osrec $(BINARY).elf $(BINARY).srec

$(BINARY).list: $(BINARY).elf
	@#printf "  OBJDUMP $(BINARY).list\n"
	$(Q)$(OBJDUMP) -S $(BINARY).elf > $(BINARY).list

$(BINARY).elf: obj_m4
	@#printf "  LD      $(subst $(shell pwd)/,,$(@))\n"
	$(Q)$(LD) -o $(BINARY).elf $(OBJ_M4_C) $(OBJ_M4_S) $(OBJDIR_M4)/m0_bin.o $(LDFLAGS_COMMON) $(LDFLAGS_M4)

$(OBJDIR_M0)/m0.elf: obj_m0
	@#printf "  LD      $(subst $(shell pwd)/,,$(@))\n"
	$(Q)$(LD) -o $(OBJDIR_M0)/m0.elf $(OBJ_M0_C) $(OBJ_M0_S) $(LDFLAGS_COMMON) $(LDFLAGS_M0)

obj_m4: $(OBJ_M4_C) $(OBJ_M4_S) $(OBJDIR_M4)/m0_bin.o

obj_m0: $(OBJ_M0_C) $(OBJ_M0_S)

$(OBJDIR_M4)/%.o: %.c | $(OBJDIR_M4)
	@printf "  CC      $(subst $(shell pwd)/,,$(@))\n"
	$(Q)$(CC) $(CFLAGS_COMMON) $(CFLAGS_M4) -o $@ -c $<

$(OBJDIR_M4)/%.o: %.s | $(OBJDIR_M4)
	@printf "  CC      $(subst $(shell pwd)/,,$(@))\n"
	$(Q)$(CC) $(CFLAGS_COMMON) $(CFLAGS_M4) -o $@ -c $<

$(OBJDIR_M4)/m0_bin.o: m0_bin.s $(OBJDIR_M0)/m0.bin | $(OBJDIR_M4)
	@printf "  CC      $(subst $(shell pwd)/,,$(@))\n"
	$(Q)$(CC) $(CFLAGS_COMMON) $(CFLAGS_M4) -o $@ -c $<

$(OBJDIR_M0)/%.o: %.c | $(OBJDIR_M0)
	@printf "  CC      $(subst $(shell pwd)/,,$(@))\n"
	$(Q)$(CC) $(CFLAGS_COMMON) $(CFLAGS_M0) -o $@ -c $<

$(OBJDIR_M0)/%.o: %.s | $(OBJDIR_M0)
	@printf "  CC      $(subst $(shell pwd)/,,$(@))\n"
	$(Q)$(CC) $(CFLAGS_COMMON) $(CFLAGS_M0) -o $@ -c $<

$(OBJDIR_M4):
	$(Q)mkdir -p $@

$(OBJDIR_M0):
	$(Q)mkdir -p $@

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
	$(Q)rm -f $(PATH_HACKRF_FIRMWARE)/hackrf_usb/*.o
	$(Q)rm -f $(PATH_HACKRF_FIRMWARE)/hackrf_usb/*.d
	$(Q)rm -f $(PATH_HACKRF_FIRMWARE)/hackrf_usb/*.lst
	$(Q)rm -f $(PATH_HACKRF_FIRMWARE_COMMON)/*.o
	$(Q)rm -f $(PATH_HACKRF_FIRMWARE_COMMON)/*.d
	$(Q)rm -f $(PATH_HACKRF_FIRMWARE_COMMON)/*.lst
	$(Q)rm -f $(PATH_HACKRF_FIRMWARE_COMMON)/xapp058/*.o
	$(Q)rm -f $(PATH_HACKRF_FIRMWARE_COMMON)/xapp058/*.d
	$(Q)rm -f $(OBJDIR_M4)/*.o
	$(Q)rm -f $(OBJDIR_M4)/*.d
	$(Q)rm -f $(OBJDIR_M4)/*.elf
	$(Q)rm -f $(OBJDIR_M4)/*.bin
	$(Q)rm -f $(OBJDIR_M4)/*.map
	$(Q)rm -f $(OBJDIR_M0)/*.o
	$(Q)rm -f $(OBJDIR_M0)/*.d
	$(Q)rm -f $(OBJDIR_M0)/*.elf
	$(Q)rm -f $(OBJDIR_M0)/*.bin
	$(Q)rm -f $(OBJDIR_M0)/*.map
	$(Q)rm -rf $(BUILD_DIR)

.PHONY: images clean

-include $(OBJ_M4_C:.o=.d)
-include $(OBJ_M0_C:.o=.d)
