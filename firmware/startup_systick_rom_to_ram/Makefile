# Hey Emacs, this is a -*- makefile -*-

BINARY = startup_systick

SRC = $(BINARY).c \
	../common/hackrf_core.c \
	../common/si5351c.c

LDSCRIPT = ../common/LPC4330_M4_rom_to_ram.ld
include ../common/Makefile_inc.mk
