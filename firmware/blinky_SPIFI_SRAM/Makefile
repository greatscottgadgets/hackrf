# Hey Emacs, this is a -*- makefile -*-

# Target file name (without extension).
TARGET = blinky

# List C source files here. (C dependencies are automatically generated.)
SRC = $(TARGET).c \
	$(LIBS_PATH)/LPC43xx_M4_Startup_ROM_to_RAM.c \
	$(LIBS_PATH)/LPC43xx_M4_Interrupts.c \
	$(LIBS_PATH)/hackrf_core.c

# Override Linker Script
LINKER_SCRIPT = LPC4330_M4_ROM_to_RAM.ld
include ../common/Makefile_inc.mk
