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

/* Linker script for HackRF Jellybean (LPC4330, 1M SPI flash, 264K SRAM). */

MEMORY
{
	/* Physical address in Flash used to copy Code from Flash to RAM */
	rom_flash (rx)  : ORIGIN = 0x80000000, LENGTH =  1M
	/* rom is really the shadow region that points to SPI flash or elsewhere */
	rom (rx)  : ORIGIN = 0x00000000, LENGTH =  1M
	ram (rwx) : ORIGIN = 0x10000000, LENGTH =  128K
	/* there are some additional RAM regions for data */
	ram_data (rw) : ORIGIN = 0x10080000, LENGTH =  72K
	ram_ahb    (rwx) : ORIGIN = 0x20000000, LENGTH =  64K
}

/* Include the common ld script. */
INCLUDE libopencm3_lpc43xx_rom_to_ram.ld

INCLUDE usbram.ld
