/*
 * Copyright 2010 - 2012 Michael Ossmann
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
/*
  Copyright 2010-07 By Opendous Inc. (www.MicropendousX.org)
  NVIC handler info copied from NXP User Manual UM10360

  Basic interrupt handlers and NVIC interrupt handler
  function table for the LPC17xx.  See TODOs for
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


/* Reset_Handler variables defined in linker script */
extern unsigned long _StackTop;

extern void Reset_Handler(void);

/* Default interrupt handler */
static void Default_Handler(void) { while(1) {;} }

/* Empty handlers aliased to the default handler */
void NMI_Handler(void) __attribute__ ((weak, alias ("Default_Handler")));
void HardFault_Handler(void) __attribute__ ((weak, alias ("Default_Handler")));
void MemManagement_Handler(void) __attribute__ ((weak, alias ("Default_Handler")));
void BusFault_Handler(void) __attribute__ ((weak, alias ("Default_Handler")));
void UsageFault_Handler(void) __attribute__ ((weak, alias ("Default_Handler")));
void SVC_Handler(void) __attribute__ ((weak, alias ("Default_Handler")));
void DebugMon_Handler(void) __attribute__ ((weak, alias ("Default_Handler")));
void PendSV_Handler(void) __attribute__ ((weak, alias ("Default_Handler")));
void SysTick_Handler(void) __attribute__ ((weak, alias ("Default_Handler")));
void DAC_IRQHandler(void) __attribute__ ((weak, alias ("Default_Handler")));
void M0CORE_IRQHandler(void) __attribute__ ((weak, alias ("Default_Handler")));
void DMA_IRQHandler(void) __attribute__ ((weak, alias ("Default_Handler")));
void ETHERNET_IRQHandler(void) __attribute__ ((weak, alias ("Default_Handler")));
void SDIO_IRQHandler(void) __attribute__ ((weak, alias ("Default_Handler")));
void LCD_IRQHandler(void) __attribute__ ((weak, alias ("Default_Handler")));
void USB0_IRQHandler(void) __attribute__ ((weak, alias ("Default_Handler")));
void USB1_IRQHandler(void) __attribute__ ((weak, alias ("Default_Handler")));
void SCT_IRQHandler(void) __attribute__ ((weak, alias ("Default_Handler")));
void RITIMER_IRQHandler(void) __attribute__ ((weak, alias ("Default_Handler")));
void TIMER0_IRQHandler(void) __attribute__ ((weak, alias ("Default_Handler")));
void TIMER1_IRQHandler(void) __attribute__ ((weak, alias ("Default_Handler")));
void TIMER2_IRQHandler(void) __attribute__ ((weak, alias ("Default_Handler")));
void TIMER3_IRQHandler(void) __attribute__ ((weak, alias ("Default_Handler")));
void MCPWM_IRQHandler(void) __attribute__ ((weak, alias ("Default_Handler")));
void ADC0_IRQHandler(void) __attribute__ ((weak, alias ("Default_Handler")));
void I2C0_IRQHandler(void) __attribute__ ((weak, alias ("Default_Handler")));
void I2C1_IRQHandler(void) __attribute__ ((weak, alias ("Default_Handler")));
void SPI_IRQHandler(void) __attribute__ ((weak, alias ("Default_Handler")));
void ADC1_IRQHandler(void) __attribute__ ((weak, alias ("Default_Handler")));
void SSP0_IRQHandler(void) __attribute__ ((weak, alias ("Default_Handler")));
void SSP1_IRQHandler(void) __attribute__ ((weak, alias ("Default_Handler")));
void USART0_IRQHandler(void) __attribute__ ((weak, alias ("Default_Handler")));
void UART1_IRQHandler(void) __attribute__ ((weak, alias ("Default_Handler")));
void USART2_IRQHandler(void) __attribute__ ((weak, alias ("Default_Handler")));
void USART3_IRQHandler(void) __attribute__ ((weak, alias ("Default_Handler")));
void I2S0_IRQHandler(void) __attribute__ ((weak, alias ("Default_Handler")));
void I2S1_IRQHandler(void) __attribute__ ((weak, alias ("Default_Handler")));
void SPIFI_IRQHandler(void) __attribute__ ((weak, alias ("Default_Handler")));
void SGPIO_IRQHandler(void) __attribute__ ((weak, alias ("Default_Handler")));
void PIN_INT0_IRQHandler(void) __attribute__ ((weak, alias ("Default_Handler")));
void PIN_INT1_IRQHandler(void) __attribute__ ((weak, alias ("Default_Handler")));
void PIN_INT2_IRQHandler(void) __attribute__ ((weak, alias ("Default_Handler")));
void PIN_INT3_IRQHandler(void) __attribute__ ((weak, alias ("Default_Handler")));
void PIN_INT4_IRQHandler(void) __attribute__ ((weak, alias ("Default_Handler")));
void PIN_INT5_IRQHandler(void) __attribute__ ((weak, alias ("Default_Handler")));
void PIN_INT6_IRQHandler(void) __attribute__ ((weak, alias ("Default_Handler")));
void PIN_INT7_IRQHandler(void) __attribute__ ((weak, alias ("Default_Handler")));
void GINT0_IRQHandler(void) __attribute__ ((weak, alias ("Default_Handler")));
void GINT1_IRQHandler(void) __attribute__ ((weak, alias ("Default_Handler")));
void EVENTROUTER_IRQHandler(void) __attribute__ ((weak, alias ("Default_Handler")));
void C_CAN1_IRQHandler(void) __attribute__ ((weak, alias ("Default_Handler")));
void ATIMER_IRQHandler(void) __attribute__ ((weak, alias ("Default_Handler")));
void RTC_IRQHandler(void) __attribute__ ((weak, alias ("Default_Handler")));
void WWDT_IRQHandler(void) __attribute__ ((weak, alias ("Default_Handler")));
void C_CAN0_IRQHandler(void) __attribute__ ((weak, alias ("Default_Handler")));
void QEI_IRQHandler(void) __attribute__ ((weak, alias ("Default_Handler")));

/*
 * This table must be appropriately placed using a linker script:
	.text :
	{
		KEEP(*(.irq_handler_table))
		*(.text*)
		*(.rodata*)
	} > rom
*/

/* TODO - simply name a handler function in your code the same as a handler
 * in the following table and it will override the default empty handler.
 * Your function should be of the form: static void Something_Handler(void) {}
 */
__attribute__ ((section(".irq_handler_table")))
void (* const _NVIC_Handler_Functions[])(void) =
{
	// Cortex-M4 Interrupts: IRQ Number - Exception Number - Offset - Vector Description
	(void *)&_StackTop,		//        0x00	Initial SP Value - defined in Linker Script
	Reset_Handler,			//     1  0x04	Reset Handler
	NMI_Handler,			// -14 2  0x08	Non-Maskable Interrupt Handler
	HardFault_Handler,		// -13 3  0x0C	Hard Fault Handler
	MemManagement_Handler,		// -12 4  0x10	Memory Management Fault Handler
	BusFault_Handler,		// -11 5  0x14	Bus Fault Handler
	UsageFault_Handler,		// -10 6  0x18	Usage Fault Handler
	0,				//     7	Reserved
	0,				//     8	Reserved
	0,				//     9	Reserved
	0,				//     10 0x2C	Reserved
	SVC_Handler,			//     11	SVCall Handler
	DebugMon_Handler,		//     12	Debug Monitor Handler
	0,				// -2  13 0x38	Reserved
	PendSV_Handler,			// -1  14 0x3C	PendSV Handler
	SysTick_Handler,		//  0  15 0x40	SysTick Handler


	/* from table 21, section 7.6.1, UM10503 LPC43xx User Manual */
	DAC_IRQHandler,         //  0 16 0x40
	M0CORE_IRQHandler,      //  1 17 0x44 Cortex-M0; Latched TXEV; for M4-M0 communication
	DMA_IRQHandler,         //  2 18 0x48
	0,                      //  3 19 0x4C reserved
	0,                      //  4 20 0x50 reserved
	ETHERNET_IRQHandler,    //  5 21 0x54
	SDIO_IRQHandler,        //  6 22 0x58
	LCD_IRQHandler,         //  7 23 0x5C
	USB0_IRQHandler,        //  8 24 0x60
	USB1_IRQHandler,        //  9 25 0x64
	SCT_IRQHandler,         // 10 26 0x68 SCT combined interrupt
	RITIMER_IRQHandler,     // 11 27 0x6C
	TIMER0_IRQHandler,      // 12 28 0x70
	TIMER1_IRQHandler,      // 13 29 0x74
	TIMER2_IRQHandler,      // 14 30 0x78
	TIMER3_IRQHandler,      // 15 31 0x7C
	MCPWM_IRQHandler,       // 16 32 0x80
	ADC0_IRQHandler,        // 17 33 0x84
	I2C0_IRQHandler,        // 18 34 0x88
	I2C1_IRQHandler,        // 19 35 0x8C
	SPI_IRQHandler,         // 20 36 0x90
	ADC1_IRQHandler,        // 21 37 0x94
	SSP0_IRQHandler,        // 22 38 0x98
	SSP1_IRQHandler,        // 23 39 0x9C
	USART0_IRQHandler,      // 24 40 0xA0
	UART1_IRQHandler,       // 25 41 0xA4 Combined UART interrupt with Modem interrupt
	USART2_IRQHandler,      // 26 42 0xA8
	USART3_IRQHandler,      // 27 43 0xAC Combined USART interrupt with IrDA interrupt
	I2S0_IRQHandler,        // 28 44 0xB0
	I2S1_IRQHandler,        // 29 45 0xB4
	SPIFI_IRQHandler,       // 30 46 0xB8
	SGPIO_IRQHandler,       // 31 47 0xBC
	PIN_INT0_IRQHandler,    // 32 48 0xC0 GPIO pin interrupt 0
	PIN_INT1_IRQHandler,    // 33 49 0xC4 GPIO pin interrupt 1
	PIN_INT2_IRQHandler,    // 34 50 0xC8 GPIO pin interrupt 2
	PIN_INT3_IRQHandler,    // 35 51 0xCC GPIO pin interrupt 3
	PIN_INT4_IRQHandler,    // 36 52 0xD0 GPIO pin interrupt 4
	PIN_INT5_IRQHandler,    // 37 53 0xD4 GPIO pin interrupt 5
	PIN_INT6_IRQHandler,    // 38 54 0xD8 GPIO pin interrupt 6
	PIN_INT7_IRQHandler,    // 39 55 0xDC GPIO pin interrupt 7
	GINT0_IRQHandler,       // 40 56 0xE0 GPIO global interrupt 0
	GINT1_IRQHandler,       // 41 57 0xE4 GPIO global interrupt 1
	EVENTROUTER_IRQHandler, // 42 58 0xE8
	C_CAN1_IRQHandler,      // 43 59 0xEC
	0,                      // 44 60 0xF0 reserved
	0,                      // 45 61 0xF4 reserved
	ATIMER_IRQHandler,      // 46 62 0xF8 Alarm timer interrupt
	RTC_IRQHandler,         // 47 63 0xFC
	0,                      // 48 64 0x100 reserved
	WWDT_IRQHandler,        // 49 65 0x104
	0,                      // 50 66 0x108 reserved
	C_CAN0_IRQHandler,      // 51 67 0x10C
	QEI_IRQHandler,         // 52 68 0x110
};
