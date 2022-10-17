#!/bin/bash
usbhub --disable-i2c --hub D9D1 power state --port 2 --reset
sleep 1s
dfu-util --device 1fc9:000c --alt 0 --download firmware/hackrf_usb/build/hackrf_usb.dfu
EXIT_CODE="$?"
if [ "$EXIT_CODE" == "0" ]
then
    echo "DFU installation success! Exiting.."
    exit $EXIT_CODE
elif [ "$EXIT_CODE" == "74" ]
then
    echo "No DFU capable USB device available! Disconnected? Exiting.."
    exit 1
elif [ "$EXIT_CODE" == "127" ]
then
    echo "dfu-util installation failed! Exiting.."
    exit $EXIT_CODE
else
    echo "god have mercy on your soul"
    exit $EXIT_CODE
fi