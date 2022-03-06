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

class hackrf_rx_context(Structure):
        _fields_ = [("length_to_receive", c_int),
                    ("remaining_to_receive", c_int),
                    ("fileno", c_int)]

def demoRXCB(hackrf_transfer):
    #print("demoRXCB")
    user_rx_context = cast(hackrf_transfer.contents.rx_ctx, POINTER(hackrf_rx_context))
    rx_buffer_length = hackrf_transfer.contents.valid_length
    rtr = user_rx_context.contents.remaining_to_receive
    if (rtr > rx_buffer_length):
        data = bytearray(cast(hackrf_transfer.contents.buffer, POINTER(c_ubyte*rx_buffer_length)).contents)
        os.write(user_rx_context.contents.fileno,data)
        user_rx_context.contents.remaining_to_receive -= rx_buffer_length
        return 0
    else:
        data = bytearray(cast(hackrf_transfer.contents.buffer, POINTER(c_ubyte*rtr)).contents)
        os.write(user_rx_context.contents.fileno,data)    
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

theTestHackRF.setCrystalPPM(3.5)

result = theTestHackRF.setSampleRate(1000000)
if (result != LibHackRfReturnCode.HACKRF_SUCCESS):
    print("Erreur :",result, ",", HackRF.getHackRfErrorCodeName(result))

result = theTestHackRF.setFrequency(433000000)
if (result != LibHackRfReturnCode.HACKRF_SUCCESS):
    print("Erreur :",result, ",", HackRF.getHackRfErrorCodeName(result))

result = theTestHackRF.setVGAGain(8)
if (result != LibHackRfReturnCode.HACKRF_SUCCESS):
    print("Erreur :",result, ",", HackRF.getHackRfErrorCodeName(result))

result = theTestHackRF.setLNAGain(8)
if (result != LibHackRfReturnCode.HACKRF_SUCCESS):
    print("Erreur :",result, ",", HackRF.getHackRfErrorCodeName(result))

result = theTestHackRF.setAmplifierMode(LibHackRfHwMode.HW_MODE_OFF)
if (result != LibHackRfReturnCode.HACKRF_SUCCESS):
    print("Erreur :",result, ",", HackRF.getHackRfErrorCodeName(result))

#
#   RX test
#

out_file = open("example4_dataout.raw","ab")

rx_context = hackrf_rx_context()
rx_context.length_to_receive = 16000000
rx_context.remaining_to_receive = 16000000
rx_context.fileno = out_file.fileno()

print(datetime.datetime.now())
result = theTestHackRF.startRX(demoRXCB,rx_context)
if (result != LibHackRfReturnCode.HACKRF_SUCCESS):
    print("Erreur :",result, ",", HackRF.getHackRfErrorCodeName(result))

while theTestHackRF.isStreaming():
    time.sleep(0)
print(datetime.datetime.now())

result = theTestHackRF.stopRX()
if (result != LibHackRfReturnCode.HACKRF_SUCCESS):
    print("Erreur :",result, ",", HackRF.getHackRfErrorCodeName(result))

result = theTestHackRF.close()
if (result != LibHackRfReturnCode.HACKRF_SUCCESS):
    print("Erreur :",result, ",", HackRF.getHackRfErrorCodeName(result))

result = HackRF.deinitialize()
if (result != LibHackRfReturnCode.HACKRF_SUCCESS):
    print("Erreur :",result, ",", HackRF.getHackRfErrorCodeName(result))


