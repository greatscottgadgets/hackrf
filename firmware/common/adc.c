/*
 * Copyright 2025 Great Scott Gadgets <info@greatscottgadgets.com>
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

#include "adc.h"
#include <libopencm3/lpc43xx/adc.h>
#include <libopencm3/lpc43xx/scu.h>

uint16_t adc_read(uint8_t pin)
{
	bool alt_pin = (pin & 0x80);
	pin &= ~0x80;
	uint8_t pin_mask = (1 << pin);
	if (alt_pin) {
		SCU_ENAIO0 |= pin_mask;
	} else {
		SCU_ENAIO0 &= ~pin_mask;
	}
	ADC0_CR = ADC_CR_SEL(pin_mask) | ADC_CR_CLKDIV(45) | ADC_CR_PDN | ADC_CR_START(1);
	while (!(ADC0_GDR & ADC_DR_DONE) || (((ADC0_GDR >> 24) & 0x7) != pin))
		;
	return (ADC0_GDR >> 6) & 0x03FF;
}

void adc_off(void)
{
	ADC0_CR = 0;
}
