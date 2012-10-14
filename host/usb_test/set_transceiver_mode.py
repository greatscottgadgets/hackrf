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

import sys
import usb

device = usb.core.find(idVendor=0x1d50, idProduct=0x604b)
device.set_configuration()

def set_rx():
    device.ctrl_transfer(0x40, 1, 1, 0)

def set_tx():
    device.ctrl_transfer(0x40, 1, 2, 0)

if len(sys.argv) == 2:
    if sys.argv[1] == 'tx':
        set_tx()
    elif sys.argv[1] == 'rx':
        set_rx()
