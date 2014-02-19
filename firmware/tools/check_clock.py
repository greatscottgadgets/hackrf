#!/usr/bin/env python

# Copyright 2013 Jared Boone
#
# Interpret the LPC43xx Clock Generation Unit (CGU) FREQ_MON register
# and display the estimated clock frequency.
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
Inside GDB:
	(gdb) set {int}0x40050014 = 0x09800000
	(gdb) x 0x40050014
	0x40050014:     0x091bd000
This script:
	./check_clock.py 0x<register value> 0x<RCNT initial value>
"""

import sys

clock_source_names = {
	0x00: '32 kHz oscillator',
	0x01: 'IRC',
	0x02: 'ENET_RX_CLK',
	0x03: 'ENET_TX_CLK',
	0x04: 'GP_CLKIN',
	0x06: 'Crystal oscillator',
	0x07: 'PLL0USB',
	0x08: 'PLL0AUDIO',
	0x09: 'PLL1',
	0x0c: 'IDIVA',
	0x0d: 'IDIVB',
	0x0e: 'IDIVC',
	0x0f: 'IDIVD',
	0x10: 'IDIVE',
}

reg_value = int(sys.argv[1], 16)
rcnt = int(sys.argv[2], 16)

print('0x%08x' % reg_value)

rcnt_final = reg_value & 0x1ff
fcnt = (reg_value >> 9) & 0x3fff
clock_source = (reg_value >> 24) & 0x1f
fref = 12e6

if rcnt_final != 0:
	raise RuntimeError('RCNT did not reach 0')

print('RCNT: %d' % rcnt)
print('FCNT: %d' % fcnt)
print('Fref: %d' % fref)

clock_hz = fcnt / float(rcnt) * fref
clock_mhz = clock_hz / 1e6
clock_name = clock_source_names[clock_source] if clock_source in clock_source_names else 'Reserved'
print('%s: %.3f MHz' % (clock_name, clock_mhz))
