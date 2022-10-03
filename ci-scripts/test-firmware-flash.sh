#!/bin/bash
host/build/hackrf-tools/src/hackrf_spiflash -w firmware/hackrf_usb/build/hackrf_usb.bin -d $EUT
EXIT_CODE="$?"
if [ "$EXIT_CODE" == "1" ]
then
    echo "No GreatFET found! Disconnected? Exiting.."
    exit $EXIT_CODE
elif [ "$EXIT_CODE" == "0" ]
then
    echo "Firmware successfully flashed!"
elif [ "$EXIT_CODE" == "127" ]
then
    echo "Host tool installation failed! Exiting.."
    exit $EXIT_CODE
else
    echo "god have mercy on your soul"
    exit $EXIT_CODE
fi