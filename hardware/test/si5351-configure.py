#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# Copyright 2012 Jared Boone
#
# This file is part of HackRF.
#
# This is a free hardware design; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#  
# This design is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#  
# You should have received a copy of the GNU General Public License
# along with this design; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street,
# Boston, MA 02110-1301, USA.
#

# This program configures a Silicon Labs Si5351C clock generation IC
# via an I2C interface, using a Dangerous Prototypes Bus Pirate v3.5
# and the pyBusPirateLite Python library.
#
# Si5351C:
#   http://www.silabs.com/products/clocksoscillators/clock-generators-and-buffers/Pages/clock+vcxo.aspx
# Bus Pirate:
#   http://dangerousprototypes.com/docs/Bus_Pirate
# pyBusPirateLite:
#   http://code.google.com/p/the-bus-pirate/
#
# This Si5351C configuration is a work in progress, it does not yet
# implement the clock plan described in the HackRF wiki:
# https://github.com/mossmann/hackrf/wiki/Clocking
#
# To follow progress in configuring the Si5351C, please refer to the
# HackRF wiki:
# https://github.com/mossmann/hackrf/wiki/Lemondrop-Bring-Up

import sys

from pyBusPirateLite.I2C import *
from math import log

if len(sys.argv) < 2:
    print('Usage: %s <path to serial device>' % (sys.argv[0],))
    sys.exit()

serial_path = sys.argv[1]
print('Connecting to Bus Pirate on "%s"...' % (serial_path,))

i2c = I2C(serial_path, 115200)
i2c.resetBP()

print('Entering I2C mode...')
if not i2c.BBmode():
    print('BBmode() failed')
    sys.exit()

if not i2c.enter_I2C():
    print('enter_I2C() failed')
    sys.exit()

if not i2c.set_speed(I2CSpeed._400KHZ):
    print('set_speed() failed')
    sys.exit()

i2c.cfg_pins(I2CPins.POWER)
i2c.timeout(0.2)

def write_registers(first_register_number, values):
    data = [0xC0, first_register_number]
    if isinstance(values, (list, tuple)):
        data.extend(values)
    else:
        data.append(values)
    i2c.send_start_bit()
    i2c.bulk_trans(len(data), data)
    i2c.send_stop_bit()

# r is the R output divider (should be 1, 2, 4, 8. . .)
# note: si5351c.c's r parameter is encoded (log not taken)
# use p2=0 and p3=1 for integer mode
def set_multisynth_parameters(ms_n, p1, p2, p3, r):
    register_number = 42 + (ms_n * 8)
    values = (
        (p3 >> 8) & 0xFF,
        (p3 >> 0) & 0xFF,
        (int(log(r, 2)) << 4) | (0 << 2) | ((p1 >> 16) & 0x3),
        (p1 >> 8) & 0xFF,
        (p1 >> 0) & 0xFF,
        (((p3 >> 16) & 0xF) << 4) | (((p2 >> 16) & 0xF) << 0),
        (p2 >> 8) & 0xFF,
        (p2 >> 0) & 0xFF
    )
    write_registers(register_number, values)

# Get the appropriate P1 setting for a given frequency.
# Assumes VCO is 800 MHz and you want integer division and R=4.
def integer_p1(frequency):
	return int(800e6/frequency) * 128 - 512

def set_codec_rate(frequency):
    set_multisynth_parameters(1, integer_p1(frequency * 4), 0, 1, 4)

print('Configuring Si5351...')

# Disable all CLKx outputs.
write_registers(3, 0xFF)

# Turn off OEB pin control for all CLKx
write_registers(9, 0xFF)

# Power down all CLKx
write_registers(16, (0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0xC0, 0xC0))

# Register 183: Crystal Internal Load Capacitance
# Reads as 0xE4 on power-up
# Set to 10pF (until I find out what loading the crystal/PCB likes best)
write_registers(183, 0xE4)

# Register 187: Fanout Enable
# Turn on XO and MultiSynth fanout only.
write_registers(187, 0x50)

# Register 15: PLL Input Source
# CLKIN_DIV=0 (Divide by 1)
# PLLB_SRC=0 (XTAL input)
# PLLA_SRC=0 (XTAL input)
write_registers(15, 0x00)

# MultiSynth NA (PLL1)
write_registers(26, (0x00, 0x01, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x00))

# MultiSynth NB (PLL2)
###

# MultiSynth 0
# This is the source for the MAX2837 clock input.
# It is also used to generate the ADC/DAC clocks.
set_multisynth_parameters(0, integer_p1(80e6), 0, 1, 2) # 40MHz with R=2

# MultiSynth 1 (MAX5864 and CPLD)
#set_codec_rate(20e6)
set_multisynth_parameters(1, integer_p1(80e6), 0, 1, 4) # 20MHz with R=2

# MultiSynth 2 (CPLD)
set_multisynth_parameters(2, integer_p1(80e6), 0, 1, 1) # 20MHz with R=2

# MultiSynth 3 (CPLD)
set_multisynth_parameters(3, integer_p1(80e6), 0, 1, 1) # 20MHz with R=2

# MultiSynth 4
# This is the source for the LPC43xx external clock input.
set_multisynth_parameters(4, 8021, 1, 3, 1)    # 12MHz
#set_multisynth_parameters(4, integer_p1(20e6), 0, 1, 1) # 20 MHz
#set_multisynth_parameters(4, integer_p1(80e6), 0, 1, 4) # 20 MHz using R=4
#set_multisynth_parameters(4, 3584, 0, 1, 1)    # 25MHz

# MultiSynth 6/7 R dividers
write_registers(92, 0x00)

# Registers 16 through 23: CLKx Control
# CLK0:
#   CLK0_PDN=0 (powered up)
#   MS0_INT=1 (integer mode)
#   MS0_SRC=0 (PLLA as source for MultiSynth 0)
#   CLK0_INV=0 (not inverted)
#   CLK0_SRC=3 (MS0 as input source)
#   CLK0_IDRV=3 (8mA)
# CLK1:
#   CLK1_PDN=0 (powered up)
#   MS1_INT=1 (integer mode)
#   MS1_SRC=0 (PLLA as source for MultiSynth 1)
#   CLK1_INV=0 (not inverted)
#   CLK1_SRC=2 (MS0 as input source)
#   CLK1_IDRV=3 (8mA)
# CLK2:
#   CLK2_PDN=0 (powered up)
#   MS2_INT=1 (integer mode)
#   MS2_SRC=0 (PLLA as source for MultiSynth 1)
#   CLK2_INV=0 (not inverted)
#   CLK2_SRC=2 (MS0 as input source)
#   CLK2_IDRV=3 (8mA)
# CLK3:
#   CLK3_PDN=0 (powered up)
#   MS3_INT=1 (integer mode)
#   MS3_SRC=0 (PLLA as source for MultiSynth 1)
#   CLK3_INV=0 (not inverted)
#   CLK3_SRC=2 (MS0 as input source)
#   CLK3_IDRV=3 (8mA)
# CLK4:
#   CLK4_PDN=0 (powered up)
#   MS4_INT=0 (fractional mode -- to support 12MHz to LPC for USB DFU)
#   MS4_SRC=0 (PLLA as source for MultiSynth 4)
#   CLK4_INV=0 (not inverted)
#   CLK4_SRC=3 (MS4 as input source)
#   CLK4_IDRV=3 (8mA)
write_registers(16, (0x4F, 0x4B, 0x4B, 0x4B, 0x0F, 0x80, 0xC0, 0xC0))

# Enable CLK outputs 0, 1, 4 only.
write_registers(3, 0xFF ^ 0b00011111)

raw_input("<return> to quit...")

print('Powering down...')
i2c.cfg_pins(0)
