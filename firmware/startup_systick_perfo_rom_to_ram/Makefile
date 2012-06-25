# Hey Emacs, this is a -*- makefile -*-

BINARY = startup_systick_perfo_rom_to_ram

SRC = startup_systick.c \
	perf_mips.c \
	../common/hackrf_core.c \
	../common/si5351c.c

LDSCRIPT = ../common/LPC4330_M4_rom_to_ram.ld
include ../common/Makefile_inc.mk
