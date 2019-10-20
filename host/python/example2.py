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
import time, threading

global n

#
#   dummy callback, not actually transmitting anything
#
def dummyTXCB(hackrf_transfer):
    global n
    #print("dummy TX")
    n = n-1
    print("TX Callback thread id : {:08x}".format(threading.current_thread().ident))
    if (n != 0):
        return 0
    else:
        # It's possible to retrive the raw or Python wrapper instance from within the callback
        # mind thread safety while using HackRF methods from within the callback as it's already
        # executing in a different thread than the one that called startTX/startRX
        pdevice = hackrf_transfer.contents.device
        theHackRf = HackRF.getInstanceByDeviceHandle(pdevice)
        theHackRf.printBoardInfos()
        return 1

#
#   dummy callback, not actually receiving anything
#
def dummyRXCB(hackrf_transfer):
    global n
    #print("dummy RX")
    n = n-1
    print("RX Callback thread id : {:08x}".format(threading.current_thread().ident))
    if (n != 0):
        return 0
    else:
        # It's possible to retrive the raw or Python wrapper instance from within the callback
        # mind thread safety while using HackRF methods from within the callback as it's already
        # executing in a different thread than the one that called startTX/startRX
        pdevice = hackrf_transfer.contents.device
        theHackRf = HackRF.getInstanceByDeviceHandle(pdevice)
        theHackRf.printBoardInfos()
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

result = theTestHackRF.setBasebandFilterBandwidth(2000000)
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
#   Dummy TX test
#
n = 5
print("Main thread id        : {:08x}".format(threading.current_thread().ident))
result = theTestHackRF.startTX(dummyTXCB,None)
if (result != LibHackRfReturnCode.HACKRF_SUCCESS):
    print("Erreur :",result, ",", HackRF.getHackRfErrorCodeName(result))

while theTestHackRF.isStreaming():
    time.sleep(0)

result = theTestHackRF.stopTX()
if (result != LibHackRfReturnCode.HACKRF_SUCCESS):
    print("Erreur :",result, ",", HackRF.getHackRfErrorCodeName(result))

#
#   Dummy RX test
#
n = 5
print("Main thread id        : {:08x}".format(threading.current_thread().ident))
result = theTestHackRF.startRX(dummyRXCB,None)
if (result != LibHackRfReturnCode.HACKRF_SUCCESS):
    print("Erreur :",result, ",", HackRF.getHackRfErrorCodeName(result))

while theTestHackRF.isStreaming():
    time.sleep(0)

result = theTestHackRF.stopRX()
if (result != LibHackRfReturnCode.HACKRF_SUCCESS):
    print("Erreur :",result, ",", HackRF.getHackRfErrorCodeName(result))

result = theTestHackRF.close()
if (result != LibHackRfReturnCode.HACKRF_SUCCESS):
    print("Erreur :",result, ",", HackRF.getHackRfErrorCodeName(result))

result = HackRF.deinitialize()
if (result != LibHackRfReturnCode.HACKRF_SUCCESS):
    print("Erreur :",result, ",", HackRF.getHackRfErrorCodeName(result))


