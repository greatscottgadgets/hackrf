/*
 * Copyright 2012-2014 Great Scott Gadgets <info@greatscottgadgets.com>
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

/* Top-level linker script for HackRF One (LPC4320, 1M SPI flash, 200K SRAM). */

/* Depending on the target, the following additional linker scripts are called:
 *
 * `hackrf_usb.bin`:
 *   - ${MCU_PARTNO}_M4_memory.ld       -> This script
 *   - LPC43xx_M4_memory.ld             -> Defines additional memory regions
 *   - LPC43xx_M4_M0_image_from_text.ld -> Adds the M0 core's firmware
 *   - LPC43xx_M4_memory_rom_only.ld    -> Adds the fpga bitstreams
 *   - LPC43xx_M4_rtconfig.ld           -> Adds code stored in ram_rtconfig
 *   - libopencm3_lpc43xx_m0.ld         -> Controls linking for M0 code
 *   - libopencm3_lpc43xx_rom_to_ram.ld -> Controls linking for M4 and shadow region
 *
 * `hackrf_usb.dfu`:
 *   - ${MCU_PARTNO}_M4_memory.ld       -> This script
 *   - LPC43xx_M4_memory.ld             -> as above
 *   - LPC43xx_M0_memory.ld             -> as above
 *   - LPC43xx_M4_M0_image_from_text.ld -> as above
 *   - LPC43xx_M4_rtconfig.ld           -> as above
 *   - libopencm3_lpc43xx.ld            -> Controls linking for M4
 *   - libopencm3_lpc43xx_m0.ld         -> as above
 */

MEMORY
{
	/* UM10503 Rev. 2.5, 10 September 2019
	 *   - Section 3.3.1  Table 10, LPC43xx/LPC43Sxx SRAM configuration
	 *   - Section 3.3.3  On-chip flash
	 *   - Section 3.3.5  Memory retention in the Power-down modes
	 *   - Section 3.4    Figure 8, Flashless parts: Memory map with peripherals
	 *   - Section 5      LPC43xx Boot ROM
	 * LPC4350/30/20/10 Product data sheet Rev. 4.6, 14 March 2016
	 *   - Section 7.17   Figure 7, LPC4350/30/20/10 Memory mapping (overview)
	 */

	/* - 0x0000 0000 : 256 MB shadow area ---------------------------------- */
	/* The shadow area allows some LPC43xx memory regions to be remapped to
	 * address 0x0000 0000.
	 *
	 * - The default value at RESET is:
	 *   -> Factory boot ROM @ 0x1040 0000
	 * - Once the boot ROM has initialized SPIFI and established the boot map
	 *   it is re-mapped to our 1M SPI Flash:
	 *   -> SPIFI data       @ 0x1400 0000
	 * - Next, the `.text_ram` region is copied from SPIFI data to `ram_local1`.
	 * - Finally, execution jumps to `ram_local1` and the firmware begins.
	 */
	rom          (rx)  : ORIGIN = 0x00000000, LENGTH =  1M

	/* - 0x1000 0000 : 96 kB local SRAM ------------------------------------ */
	ram_local1   (rwx) : ORIGIN = 0x10000000, LENGTH = 64K
	ram_usb      (rw)  : ORIGIN = 0x10010000, LENGTH = 32K

	/* - 0x1001 8000 : 32 kB local SRAM (LPC4350/30) ----------------------- */
	/* - 0x1002 0000 : reserved -------------------------------------------- */

	/* - 0x1008 0000 : 32 kB local SRAM (LPC4350/30/20/10) ----------------- */
	ram_rtconfig (rx)  : ORIGIN = 0x10080000, LENGTH = 16K
	ram_local2   (rwx) : ORIGIN = 0x10084000, LENGTH = 16K

	/* - 0x1008 8000: 8 kB local SRAM (LPC4320/10) ------------------------- */
	ram_sleep    (rwx) : ORIGIN = 0x10088000, LENGTH =  8K

	/* - 0x1008 A000 : 32 kB local SRAM (LPC4350/30) ----------------------- */
}

ASSERT(
	SIZEOF(.text) + SIZEOF(.init_array) + SIZEOF(.ARM.exidx) <= 64K,
	"Error: region `ram_local1' exceeds 64 kB."
)

INCLUDE LPC43xx_M4_memory.ld
