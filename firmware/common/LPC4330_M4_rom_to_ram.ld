/*
 * Copyright 2012 Michael Ossmann <mike@ossmann.com>
 * Copyright (C) 2012 Benjamin Vernoux <titanmkd@gmail.com>
 * Copyright 2012 Jared Boone <jared@sharebrained.com>
 *
 * This file is part of HackRF
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

/* Linker script for HackRF Jellybean/Jawbreaker (LPC4330, 1M SPI flash, 264K SRAM). */

MEMORY
{
	/* Physical address in Flash used to copy Code from Flash to RAM */
	rom_flash (rx)  : ORIGIN = 0x80000000, LENGTH =  1M
	/* rom is really the shadow region that points to SPI flash or elsewhere */
	rom (rx)  : ORIGIN = 0x00000000, LENGTH =  128K
	ram_local1 (rwx) : ORIGIN = 0x10000000, LENGTH =  128K
	ram_local2 (rwx) : ORIGIN = 0x10080000, LENGTH =  64K
	ram_sleep (rwx) : ORIGIN = 0x10090000, LENGTH = 8K
	ram_m0 (rwx) : ORIGIN = 0x20000000, LENGTH = 28K
	ram_shared (rwx) : ORIGIN = 0x20007000, LENGTH =  4K
	ram_usb0 (rwx) : ORIGIN = 0x20008000, LENGTH = 16K
	ram_usb1 (rwx) : ORIGIN = 0x2000c000, LENGTH = 16K
	/* ram_usb[01]: USB buffer. Straddles two blocks of RAM
	 * to get performance benefit of having two USB buffers addressable
	 * simultaneously (on two different buses of the AHB multilayer matrix)
	 */
}

/* Include the common ld script. */
INCLUDE libopencm3_lpc43xx_rom_to_ram.ld

SECTIONS
{
	.text : {
		PROVIDE(__m0_start__ = .);
		KEEP(*(.m0_bin*));
		. = ALIGN(4);
		PROVIDE(__m0_end__ = .);
	} >rom
}
