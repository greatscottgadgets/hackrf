/*
 * Copyright 2012 Benjamin Vernoux
 *
 * This file is part of HackRF.
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
/*
  Copyright 2010-07 By Opendous Inc. (www.MicropendousX.org)
  NVIC handler info copied from NXP User Manual UM10360

  Start-up code for LPC17xx.  See TODOs for
  modification instructions.

  Permission to use, copy, modify, and distribute this software
  and its documentation for any purpose and without fee is hereby
  granted, provided that the above copyright notice appear in all
  copies and that both that the copyright notice and this
  permission notice and warranty disclaimer appear in supporting
  documentation, and that the name of the author not be used in
  advertising or publicity pertaining to distribution of the
  software without specific, written prior permission.

  The author disclaim all warranties with regard to this
  software, including all implied warranties of merchantability
  and fitness.  In no event shall the author be liable for any
  special, indirect or consequential damages or any damages
  whatsoever resulting from loss of use, data or profits, whether
  in an action of contract, negligence or other tortious action,
  arising out of or in connection with the use or performance of
  this software.
*/

#include <lpc43.h>

#define CREG_BASE       0x40043000
#define MMIO32(addr)    (*(volatile unsigned long *)(addr))
#define CREG_M4MEMMAP   MMIO32(CREG_BASE + 0x100)

/* Reset_Handler_ROM_to_RAM variables defined in linker script */
extern unsigned long _text_ram; /* Correspond to start of Code in RAM */
extern unsigned long _etext_ram; /* Correspond to end of Code in RAM */
extern unsigned long _etext_rom; /* Correspond to end of Code in ROM */


/* Reset_Handler variables defined in linker script */
extern unsigned long _interrupt_vector_table;
extern unsigned long _data;
extern unsigned long _edata;
extern unsigned long _etext;
extern unsigned long _bss;
extern unsigned long _ebss;

extern void __libc_init_array(void);
extern int main(void);

/* Code to be Copied from ROM to RAM */
void Reset_Handler_ROM_to_RAM(void)
{
    unsigned long *src, *dest;

	// Copy the code from ROM to Real RAM
	src = &_etext_rom-(&_etext_ram-&_text_ram);
	for(dest = &_text_ram; dest < &_etext_ram; )
	{
		*dest++ = *src++;
	}

    /* Change Shadow memory to Real RAM */
    CREG_M4MEMMAP = &_text_ram;

    /* Continue Execution in RAM */
}

/* Reset Handler */
void Reset_Handler(void)
{
    unsigned long *src, *dest;

    Reset_Handler_ROM_to_RAM();

	// Copy the data segment initializers from flash to SRAM
	src = &_etext;
	for(dest = &_data; dest < &_edata; )
	{
		*dest++ = *src++;
	}

	// Initialize the .bss segment of memory to zeros
	src = &_bss;
	while (src < &_ebss)
	{
		*src++ = 0;
	}

    __libc_init_array();
    
    // Set the vector table location.
    SCB_VTOR = &_interrupt_vector_table;
    
	main();

	// In case main() fails, have something to breakpoint
	while (1) {;}
}
