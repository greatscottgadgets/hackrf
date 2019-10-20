#!/usr/bin/python3
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

from pyhackrf import *
from ctypes import *
import os, time, datetime

class hackrf_tx_context(Structure):
        _fields_ = [("buffer", POINTER(c_ubyte)),
                    ("last_tx_pos", c_int),
                    ("buffer_length", c_int) ]

def demoTXCB(hackrf_transfer):
    #print("demoTXCB")
    user_tx_context = cast(hackrf_transfer.contents.tx_ctx, POINTER(hackrf_tx_context))
    tx_buffer_length = hackrf_transfer.contents.valid_length
    left = user_tx_context.contents.buffer_length - user_tx_context.contents.last_tx_pos
    addr_dest = addressof(hackrf_transfer.contents.buffer.contents)
    addr_src = addressof(user_tx_context.contents.buffer.contents)

    if (left > tx_buffer_length):
        memmove(addr_dest,addr_src,tx_buffer_length)
        user_tx_context.contents.last_tx_pos += tx_buffer_length
        return 0
    else:
        memmove(addr_dest,addr_src,left)
        memset(addr_dest+left,0,tx_buffer_length-left)
        return 1

HackRF.setLogLevel(logging.INFO)
#HackRF.setLogLevel(logging.DEBUG)

result = HackRF.initialize()
if (result != LibHackRfReturnCode.HACKRF_SUCCESS):
    print("Erreur :",result, ",", HackRF.getHackRfErrorCodeName(result))

theTestHackRF = HackRF()

result = theTestHackRF.open()
if (result != LibHackRfReturnCode.HACKRF_SUCCESS):
    print("Erreur :",result, ",", HackRF.getHackRfErrorCodeName(result))

theTestHackRF.getCrystalPPM(3.5)

result = theTestHackRF.setSampleRate(2000000)
if (result != LibHackRfReturnCode.HACKRF_SUCCESS):
    print("Erreur :",result, ",", HackRF.getHackRfErrorCodeName(result))

result = theTestHackRF.setFrequency(433000000)
if (result != LibHackRfReturnCode.HACKRF_SUCCESS):
    print("Erreur :",result, ",", HackRF.getHackRfErrorCodeName(result))

result = theTestHackRF.setTXVGAGain(1)
if (result != LibHackRfReturnCode.HACKRF_SUCCESS):
    print("Erreur :",result, ",", HackRF.getHackRfErrorCodeName(result))

result = theTestHackRF.setAmplifierMode(LibHackRfHwMode.HW_MODE_OFF)
if (result != LibHackRfReturnCode.HACKRF_SUCCESS):
    print("Erreur :",result, ",", HackRF.getHackRfErrorCodeName(result))

#
#   TX test 4M "0" samples (2 bytes per IQ sample)
#

data = bytearray(1000*1000*8)

tx_context = hackrf_tx_context()
tx_context.buffer_length = len(data)
tx_context.last_tx_pos = 0
tx_context.buffer = (c_ubyte*tx_context.buffer_length).from_buffer_copy(data)

print(datetime.datetime.now())
result = theTestHackRF.startTX(demoTXCB,tx_context)
if (result != LibHackRfReturnCode.HACKRF_SUCCESS):
    print("Erreur :",result, ",", HackRF.getHackRfErrorCodeName(result))

while theTestHackRF.isStreaming():
    time.sleep(0)
print(datetime.datetime.now())

result = theTestHackRF.stopTX()
if (result != LibHackRfReturnCode.HACKRF_SUCCESS):
    print("Erreur :",result, ",", HackRF.getHackRfErrorCodeName(result))

result = theTestHackRF.close()
if (result != LibHackRfReturnCode.HACKRF_SUCCESS):
    print("Erreur :",result, ",", HackRF.getHackRfErrorCodeName(result))

result = HackRF.deinitialize()
if (result != LibHackRfReturnCode.HACKRF_SUCCESS):
    print("Erreur :",result, ",", HackRF.getHackRfErrorCodeName(result))


