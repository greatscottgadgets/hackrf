#!/bin/bash
set -e
usbhub --disable-i2c --hub D9D1 power state --port 1,2,3,4 $1
usbhub --disable-i2c --hub 624C power state --port 1,2,3,4 $1
