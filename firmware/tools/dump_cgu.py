#!/usr/bin/env python

# Copyright 2013 Jared Boone
#
# Display the LPC43xx Clock Generation Unit (CGU) registers in an
# easy-to-read format.
#
# This file is part of HackRF.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street,
# Boston, MA 02110-1301, USA.

"""
In GDB:
	dump binary memory cgu.bin 0x40050014 0x400500cc
"""

import sys
from struct import unpack

address = 0x40050014

f = open(sys.argv[1], 'read')
d = '\x00' * 20 + f.read()
length = len(d)
f.close()

def print_data(d):
	for i in range(0, length, 16):
		values = unpack('<IIII', d[i:i+16])
		values = ['%08x' % v for v in values]
		values_str = ' '.join(values)
		line = '%08x: %s' % (address + i, values_str)
		print(line)

#print_data(d)
#sys.exit(0)

data = {}
for i in range(0, length, 4):
	data[i] = unpack('<I', d[i:i+4])[0]

registers = {
	0x14: {
		'name': 'FREQ_MON',
		'fields': (
			('RCNT', 0, 9),
			('FCNT', 9, 14),
			('MEAS', 23, 1),
			('CLK_SEL', 24, 5),
		),
	},
	0x18: {
		'name': 'XTAL_OSC_CONTROL',
		'fields': (
			('ENABLE', 0, 1),
			('BYPASS', 1, 1),
			('HF', 2, 1)
		),
	},
	0x1c: {
		'name': 'PLL0USB_STAT',
		'fields': (
			('LOCK', 0, 1),
			('FR', 1, 1),
		),
	},
	0x20: {
		'name': 'PLL0USB_CTRL',
		'fields': (
			('PD', 0, 1),
			('BYPASS', 1, 1),
			('DIRECTI', 2, 1),
			('DIRECTO', 3, 1),
			('CLKEN', 4, 1),
			('FRM', 6, 1),
			('AUTOBLOCK', 11, 1),
			('CLK_SEL', 24, 5),
		),
	},
	0x24: {
		'name': 'PLL0USB_MDIV',
		'fields': (
			('MDEC', 0, 17),
			('SELP', 17, 5),
			('SELI', 22, 6),
			('SELR', 28, 4),
		),
	},
	0x28: {
		'name': 'PLL0USB_NP_DIV',
		'fields': (
			('PDEC', 0, 7),
			('NDEC', 12, 10),
		),
	},
	0x40: {
		'name': 'PLL1_STAT',
		'fields': (
			('LOCK', 0, 1),
		),
	},
	0x44: {
		'name': 'PLL1_CTRL',
		'fields': (
			('PD', 0, 1),
			('BYPASS', 1, 1),
			('FBSEL', 6, 1),
			('DIRECT', 7, 1),
			('PSEL', 8, 2),
			('AUTOBLOCK', 11, 1),
			('NSEL', 12, 2),
			('MSEL', 16, 8),
			('CLK_SEL', 24, 5),
		),
	},
	0x48: {
		'name': 'IDIVA_CTRL',
		'fields': (
			('PD', 0, 1),
			('IDIV', 2, 2),
			('AUTOBLOCK', 11, 1),
			('CLK_SEL', 24, 5),
		),
	},
	0x4C: {
		'name': 'IDIVB_CTRL',
		'fields': (
			('PD', 0, 1),
			('IDIV', 2, 4),
			('AUTOBLOCK', 11, 1),
			('CLK_SEL', 24, 5),
		),
	},
	0x50: {
		'name': 'IDIVC_CTRL',
		'fields': (
			('PD', 0, 1),
			('IDIV', 2, 4),
			('AUTOBLOCK', 11, 1),
			('CLK_SEL', 24, 5),
		),
	},
	0x54: {
		'name': 'IDIVD_CTRL',
		'fields': (
			('PD', 0, 1),
			('IDIV', 2, 4),
			('AUTOBLOCK', 11, 1),
			('CLK_SEL', 24, 5),
		),
	},
	0x58: {
		'name': 'IDIVE_CTRL',
		'fields': (
			('PD', 0, 1),
			('IDIV', 2, 8),
			('AUTOBLOCK', 11, 1),
			('CLK_SEL', 24, 5),
		),
	},
	0x5C: {
		'name': 'BASE_SAFE_CLK',
		'fields': (
			('PD', 0, 1),
			('AUTOBLOCK', 11, 1),
			('CLK_SEL', 24, 5),
		),
	},
	0x60: {
		'name': 'BASE_USB0_CLK',
		'fields': (
			('PD', 0, 1),
			('AUTOBLOCK', 11, 1),
			('CLK_SEL', 24, 5),
		),
	},
	0x64: {
		'name': 'BASE_PERIPH_CLK',
		'fields': (
			('PD', 0, 1),
			('AUTOBLOCK', 11, 1),
			('CLK_SEL', 24, 5),
		),
	},
	0x68: {
		'name': 'BASE_USB1_CLK',
		'fields': (
			('PD', 0, 1),
			('AUTOBLOCK', 11, 1),
			('CLK_SEL', 24, 5),
		),
	},
	0x6C: {
		'name': 'BASE_M4_CLK',
		'fields': (
			('PD', 0, 1),
			('AUTOBLOCK', 11, 1),
			('CLK_SEL', 24, 5),
		),
	},
	0x70: {
		'name': 'BASE_SPIFI_CLK',
		'fields': (
			('PD', 0, 1),
			('AUTOBLOCK', 11, 1),
			('CLK_SEL', 24, 5),
		),
	},
	0x74: {
		'name': 'BASE_SPI_CLK',
		'fields': (
			('PD', 0, 1),
			('AUTOBLOCK', 11, 1),
			('CLK_SEL', 24, 5),
		),
	},
	0x78: {
		'name': 'BASE_PHY_RX_CLK',
		'fields': (
			('PD', 0, 1),
			('AUTOBLOCK', 11, 1),
			('CLK_SEL', 24, 5),
		),
	},
	0x7C: {
		'name': 'BASE_PHY_TX_CLK',
		'fields': (
			('PD', 0, 1),
			('AUTOBLOCK', 11, 1),
			('CLK_SEL', 24, 5),
		),
	},
	0x80: {
		'name': 'BASE_APB1_CLK',
		'fields': (
			('PD', 0, 1),
			('AUTOBLOCK', 11, 1),
			('CLK_SEL', 24, 5),
		),
	},
	0x84: {
		'name': 'BASE_APB3_CLK',
		'fields': (
			('PD', 0, 1),
			('AUTOBLOCK', 11, 1),
			('CLK_SEL', 24, 5),
		),
	},
	0x88: {
		'name': 'BASE_LCD_CLK',
		'fields': (
			('PD', 0, 1),
			('AUTOBLOCK', 11, 1),
			('CLK_SEL', 24, 5),
		),
	},
	0x8C: {
		'name': 'BASE_VADC_CLK',
		'fields': (
			('PD', 0, 1),
			('AUTOBLOCK', 11, 1),
			('CLK_SEL', 24, 5),
		),
	},
	0x90: {
		'name': 'BASE_SDIO_CLK',
		'fields': (
			('PD', 0, 1),
			('AUTOBLOCK', 11, 1),
			('CLK_SEL', 24, 5),
		),
	},
	0x94: {
		'name': 'BASE_SSP0_CLK',
		'fields': (
			('PD', 0, 1),
			('AUTOBLOCK', 11, 1),
			('CLK_SEL', 24, 5),
		),
	},
	0x98: {
		'name': 'BASE_SSP1_CLK',
		'fields': (
			('PD', 0, 1),
			('AUTOBLOCK', 11, 1),
			('CLK_SEL', 24, 5),
		),
	},
	0x9C: {
		'name': 'BASE_UART0_CLK',
		'fields': (
			('PD', 0, 1),
			('AUTOBLOCK', 11, 1),
			('CLK_SEL', 24, 5),
		),
	},
	0xA0: {
		'name': 'BASE_UART1_CLK',
		'fields': (
			('PD', 0, 1),
			('AUTOBLOCK', 11, 1),
			('CLK_SEL', 24, 5),
		),
	},
	0xA4: {
		'name': 'BASE_UART2_CLK',
		'fields': (
			('PD', 0, 1),
			('AUTOBLOCK', 11, 1),
			('CLK_SEL', 24, 5),
		),
	},
	0xA8: {
		'name': 'BASE_UART3_CLK',
		'fields': (
			('PD', 0, 1),
			('AUTOBLOCK', 11, 1),
			('CLK_SEL', 24, 5),
		),
	},
	0xAC: {
		'name': 'BASE_OUT_CLK',
		'fields': (
			('PD', 0, 1),
			('AUTOBLOCK', 11, 1),
			('CLK_SEL', 24, 5),
		),
	},
	0xC0: {
		'name': 'BASE_APLL_CLK',
		'fields': (
			('PD', 0, 1),
			('AUTOBLOCK', 11, 1),
			('CLK_SEL', 24, 5),
		),
	},
	0xC4: {
		'name': 'BASE_CGU_OUT0_CLK',
		'fields': (
			('PD', 0, 1),
			('AUTOBLOCK', 11, 1),
			('CLK_SEL', 24, 5),
		),
	},
	0xC8: {
		'name': 'BASE_CGU_OUT1_CLK',
		'fields': (
			('PD', 0, 1),
			('AUTOBLOCK', 11, 1),
			('CLK_SEL', 24, 5),
		),
	},
	# TODO: Add other CGU registers. I did the ones that were
	# valuable to me to debug CPU clock issues.
}

for address in sorted(registers):
	register = registers[address]
	name = register['name']
	fields = register['fields']
	value = data[address]
	bits = bin(value)[2:].zfill(32)
	print('%03x %20s %s = %08x' % (address, name, bits, value))
	for field in fields:
		name, low_bit, count = field
		field_value = (value >> low_bit) & ((1 << count) - 1)
		field_bits = bin(field_value)[2:].zfill(count) + ' ' * low_bit
		field_bits = field_bits.rjust(32)
		print('%03s %20s %s = %8x %s' % ('', '', field_bits, field_value, name))
