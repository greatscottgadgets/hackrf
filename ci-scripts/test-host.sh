#!/bin/bash
hubs hackrf_dfu reset
sleep 1s
host/build/hackrf-tools/src/hackrf_info
EXIT_CODE="$?"
if [ "$EXIT_CODE" == "1" ]
then
    echo "Host tool installation success! Exiting.."
    exit 0
elif [ "$EXIT_CODE" == "0" ]
then
    echo "Failed to boot HackRF into DFU mode! Check DFU pin jumper. Exiting.."
    exit 1
elif [ "$EXIT_CODE" == "127" ]
then
    echo "Host tool installation failed! Exiting.."
    exit $EXIT_CODE
else
    echo "god have mercy on your soul"
    exit $EXIT_CODE
fi
