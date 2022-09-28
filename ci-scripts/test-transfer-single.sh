#!/bin/bash 
echo "Single-device hackrf_transfer tests:"
echo "Testing single sample transmit: -c0"
COMMAND=host/build/hackrf-tools/src/hackrf_transfer
timeout -s2 -k1 2 ${COMMAND} -a0 -x0 -c0 -n1 >/tmp/stdout.$$ 2>/tmp/stderr.$$
EXIT_CODE="$?"
if [ "$EXIT_CODE" == "124" ]
then
	cat /tmp/stderr.$$
	echo "Command timed out (SIGINT after 2 seconds)."
	exit $EXIT_CODE
elif [ "$EXIT_CODE" == "137" ]
then
	cat /tmp/stderr.$$
	echo "Command timed out (SIGKILL after 3 seconds)."
	exit $EXIT_CODE
elif [ "$EXIT_CODE" != "0" ]
then
	cat /tmp/stderr.$$
	echo "unexpected exit code"
	exit $EXIT_CODE
else
	echo "hackrf_transfer command completed."
fi

grep -q "Couldn't transfer any bytes for one second." /tmp/stderr.$$
if [ "$?" == "0" ]
then
	cat /tmp/stderr.$$
	exit 1
fi

grep -q "average power -inf dBfs" /tmp/stderr.$$
if [ "$?" != "0" ]
then
	cat /tmp/stderr.$$
	"unexpected power level"
	exit 2
fi

grep -q "^ 0.0 MiB" /tmp/stderr.$$
if [ "$?" != "0" ]
then
	cat /tmp/stderr.$$
	"unexpected total data transferred"
	exit 3
fi

if [ -s "/tmp/stdout.$$" ]
then
	cat /tmp/stderr.$$
	"unexpected stdout:"
	cat /tmp/stdout.$$
	exit 4
fi
