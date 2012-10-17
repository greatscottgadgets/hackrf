#!/usr/bin/env python
#
# Copyright 2012 Jared Boone
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
#

import usb
import struct

device = usb.core.find(idVendor=0x1d50, idProduct=0x604b)
device.set_configuration()

def read_max2837_register(register_number):
    return struct.unpack('<H', device.ctrl_transfer(0xC0, 3, 0, register_number, 2))[0]

def write_max2837_register(register_number, value):
    device.ctrl_transfer(0x40, 2, value, register_number)

def dump_max2837():
    for i in range(32):
        print('%2d: %03x' % (i, read_max2837_register(i)))

dump_max2837()
