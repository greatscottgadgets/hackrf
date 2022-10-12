#!/bin/bash

# Configuration for EUT and TESTER devices
# FIXME: move to Dockerfile or somewhere
EUT=RunningFromRAM
TESTER=0000000000000000325866e629a25623
EUT_HUB=D9D1
EUT_PORT=2
TESTER_HUB=624C
TESTER_PORT=2

COMMAND=host/build/hackrf-tools/src/hackrf_clock

# Power on USB ports for both EUT and TESTER
usbhub --disable-i2c --hub ${EUT_HUB} power state --port ${EUT_PORT} --on
usbhub --disable-i2c --hub ${TESTER_HUB} power state --port ${TESTER_PORT} --on
sleep 1

echo "Activating CLKOUT"
${COMMAND} -d ${EUT} -o1 >/tmp/stdout.$$ 2>/tmp/stderr.$$
if [ "$?" != "0" ]
then
	cat /tmp/stdout.$$
	cat /tmp/stderr.$$
	echo "hackrf_clock command failed."
	exit $EXIT_CODE
fi

${COMMAND} -d ${TESTER} -i >/tmp/stdout.$$ 2>/tmp/stderr.$$

grep -q ": clock signal detected" /tmp/stdout.$$
if [ "$?" != "0" ]
then
	cat /tmp/stdout.$$
	cat /tmp/stderr.$$
	exit 2
fi

echo "Deactivating CLKOUT"
${COMMAND} -d ${EUT} -o0 >/tmp/stdout.$$ 2>/tmp/stderr.$$
if [ "$?" != "0" ]
then
	cat /tmp/stdout.$$
	cat /tmp/stderr.$$
	echo "hackrf_clock command failed."
	exit $EXIT_CODE
fi

${COMMAND} -d ${TESTER} -i >/tmp/stdout.$$ 2>/tmp/stderr.$$

grep -q ": no clock signal detected" /tmp/stdout.$$
if [ "$?" != "0" ]
then
	cat /tmp/stdout.$$
	cat /tmp/stderr.$$
	exit 3
fi
