/*
 * Copyright 2012-2017 Great Scott Gadgets <info@greatscottgadgets.com>
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

/* Linker script for Rad1o badge - (LPC4330, 1M SPI flash, 264K SRAM). */

MEMORY
{
	/* UM10503 Rev. 2.5, 10 September 2019
	 *   - Section 3.3.1  Table 10, LPC43xx/LPC43Sxx SRAM configuration
	 *   - Section 3.3.3  On-chip flash
	 *   - Section 3.3.5  Memory retention in the Power-down modes
	 *   - Section 3.4    Figure 8, Flashless parts: Memory map with peripherals
	 * LPC4350/30/20/10 Product data sheet Rev. 4.6, 14 March 2016
	 *   - Section 7.17   Figure 7, LPC4350/30/20/10 Memory mapping (overview)
	 */

	/* - 0x0000 0000 : 256 MB shadow area ---------------------------------- */

	/* rom is really the shadow region that points to SPI flash or elsewhere */
	rom          (rx)  : ORIGIN = 0x00000000, LENGTH = 128K

	/* - 0x1000 0000 : 96 kB local SRAM ------------------------------------ */
	ram_local1   (rwx) : ORIGIN = 0x10000000, LENGTH =  96K

	/* - 0x1001 8000 : 32 kB local SRAM (LPC4350/30) ----------------------- */
	ram_usb      (rw)  : ORIGIN = 0x10018000, LENGTH =  32K

	/* - 0x1002 0000 : reserved -------------------------------------------- */

	/* - 0x1008 0000 : 32kB local SRAM (LPC4350/30/20/10) ------------------ */
	ram_rtconfig (rx)  : ORIGIN = 0x10080000, LENGTH =  32K

	/* - 0x1008 A000 : 32kB local SRAM (LPC4350/30) ------------------------ */
	ram_local2   (rwx) : ORIGIN = 0x1008A000, LENGTH =  32K

	/* - 0x1009 0000 : 8kB local SRAM  (LPC4350/30) ------------------------ */
	ram_sleep    (rwx) : ORIGIN = 0x10090000, LENGTH =   8K
}

ASSERT(
	SIZEOF(.text) + SIZEOF(.init_array) + SIZEOF(.ARM.exidx) <= 96K,
	"Error: region `ram_local1' exceeds 96 kB."
)

INCLUDE LPC43xx_M4_memory.ld
