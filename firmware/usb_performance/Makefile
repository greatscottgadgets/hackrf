# Hey Emacs, this is a -*- makefile -*-

BINARY = usb_performance

SRC = $(BINARY).c \
  usb.c \
  usb_request.c \
  usb_standard_request.c \
  usb_descriptor.c \
  ../common/fault_handler.c \
	../common/hackrf_core.c \
	../common/si5351c.c \
	../common/bitband.c

LDSCRIPT = ../common/LPC4330_M4_ram_only.ld
include ../common/Makefile_inc.mk
