/*
 * Copyright 2010 - 2012 Michael Ossmann
 * Copyright 2012 Benjamin Vernoux
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
OUTPUT_FORMAT ("elf32-littlearm", "elf32-bigarm", "elf32-littlearm")
ENTRY(_start)
SEARCH_DIR(.)
GROUP(libgcc.a libc.a libm.a libnosys.a)

MEMORY
{
  /*
   * Our code is installed in SPIFI(ROM) at 0x80000000 it is addressed through the 256M shadow area at 0x00000000 at Boot.
   * Reset_Handler call Reset_Handler_ROM_to_RAM(executed in SPIFI) and Copy the code from ROM to RAM,
   * and set shadow pointer to RAM, then the code execution continue in RAM.
   */
   rom (rx) : ORIGIN = 0x80000000, LENGTH = 128K /* Real ROM  Address It cannot exceed RAM size (Real Size is 1MB) */
   ram (rwx) : ORIGIN = 0x10000000, LENGTH = 128K /* Real RAM Address */
   shadow_ram (rwx)  : ORIGIN = 0x00000000, LENGTH = 128K /* 128 Kb Real Address is 0x10000000 but remapped to Shadow 0x00000000 */

  /* there are some additional RAM regions */
}

/*
 * much copied from: Linker script for Cortex-M3
 *
 * Version:CodeSourcery Sourcery G++ Lite 2007q3-53
 * BugURL:https://support.codesourcery.com/GNUToolchain/
 *
 *  Copyright 2007 CodeSourcery.
 *
 * The authors hereby grant permission to use, copy, modify, distribute,
 * and license this software and its documentation for any purpose, provided
 * that existing copyright notices are retained in all copies and that this
 * notice is included verbatim in any distributions. No written agreement,
 * license, or royalty fee is required for any of the authorized uses.
 * Modifications to this software may be copyrighted by their authors
 * and need not follow the licensing terms described here, provided that
 * the new terms are clearly indicated on the first page of each file where
 * they apply.
 */

EXTERN(Reset_Handler)
ENTRY(Reset_Handler)

SECTIONS
{
	.text :
	{
		_text_ram = . + ORIGIN(ram); /* Start of Code in RAM */

		. = ALIGN(0x400); /* Ensure that vector table is aligned as hardware requires. */
		_interrupt_vector_table = .;
		KEEP(*(.irq_handler_table))

		*(.text .text.* .gnu.linkonce.t.*)
		*(.rodata .rodata.* .gnu.linkonce.r.*)

		*(.eh_frame_hdr)
		*(.eh_frame)

		. = ALIGN(4);
		KEEP(*(.init))

		. = ALIGN(4);
		__preinit_array_start = .;
		KEEP (*(.preinit_array))
		__preinit_array_end = .;

		. = ALIGN(4);
		__init_array_start = .;
		KEEP (*(SORT(.init_array.*)))
		KEEP (*(.init_array))
		__init_array_end = .;

		. = ALIGN(0x4);
		KEEP (*crtbegin.o(.ctors))
		KEEP (*(EXCLUDE_FILE (*crtend.o) .ctors))
		KEEP (*(SORT(.ctors.*)))
		KEEP (*crtend.o(.ctors))

		. = ALIGN(4);
		KEEP(*(.fini))

		. = ALIGN(4);
		__fini_array_start = .;
		KEEP (*(.fini_array))
		KEEP (*(SORT(.fini_array.*)))
		__fini_array_end = .;

		KEEP (*crtbegin.o(.dtors))
		KEEP (*(EXCLUDE_FILE (*crtend.o) .dtors))
		KEEP (*(SORT(.dtors.*)))
		KEEP (*crtend.o(.dtors))

		. = ALIGN(8);
		_etext = .;
	} > shadow_ram

	.ARM.extab : 
	{
		*(.ARM.extab* .gnu.linkonce.armextab.*)
	} > shadow_ram

	__exidx_start = .;
	.ARM.exidx :
	{
		*(.ARM.exidx* .gnu.linkonce.armexidx.*)
	} > shadow_ram
	__exidx_end = .;

	_etext = .;
	_etext_ram = . + ORIGIN(ram);
	_etext_rom = . + ORIGIN(rom);

	.data :
	{
		_data = .;
		*(vtable)
		*(.data*)
		_edata = .;
	} > shadow_ram

	/* zero initialized data */
	.bss :
	{
		_bss = .;
		__bss_start__ = .;
		*(.bss*)
		*(COMMON)
		_ebss = .;
		__bss_end__ = .;
	} > shadow_ram

	/* Where we put the heap with cr_clib */
	.cr_heap :
	{
		end = .;
		__end__ = .;
		_pvHeapStart = .;
	} > shadow_ram

	/* Leave room above stack for IAP to run */
	_StackTop = ORIGIN(ram) + (ORIGIN(shadow_ram) + LENGTH(shadow_ram) - 32);

}
