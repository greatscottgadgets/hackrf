# Hey Emacs, this is a -*- makefile -*-

BINARY = usb_performance

SRC = $(BINARY).c \
	../common/hackrf_core.c \
	../common/si5351c.c \
	../common/bitband.c

LDSCRIPT = ../common/LPC4330_M4_ram_only.ld
include ../common/Makefile_inc.mk
