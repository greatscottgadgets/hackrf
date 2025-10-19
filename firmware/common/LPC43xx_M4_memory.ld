/*
 * Copyright 2012-2021 Great Scott Gadgets <info@greatscottgadgets.com>
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

MEMORY
{
	/* Physical address in Flash used to copy Code from Flash to RAM */
	rom_flash (rx)    : ORIGIN = 0x80000000, LENGTH = 1M
	/* Buffers for LZ4 decompression */
	ram_lz4_in (rwx)  : ORIGIN = 0x20000000, LENGTH = 4K
	ram_lz4_out (rwx) : ORIGIN = 0x20001000, LENGTH = 4K
	/* RAM for M0 code */
	ram_m0 (rwx)      : ORIGIN = 0x20002000, LENGTH = 20K
	/* RAM shared between M0 and M4 */
	ram_shared (rwx)  : ORIGIN = 0x20007000, LENGTH = 4K
	/* USB buffer. Straddles two blocks of RAM
	 * to get performance benefit of having two USB buffers addressable
	 * simultaneously (on two different buses of the AHB multilayer matrix)
	 */
	ram_samp (rwx)     : ORIGIN = 0x20008000, LENGTH = 32K
}

lz4_in_buf = ORIGIN(ram_lz4_in);
lz4_out_buf = ORIGIN(ram_lz4_out);
usb_samp_buffer = ORIGIN(ram_samp);
usb_bulk_buffer = ORIGIN(ram_usb);
m0_state = ORIGIN(ram_shared);
PROVIDE(__ram_m0_start__ = ORIGIN(ram_m0));
