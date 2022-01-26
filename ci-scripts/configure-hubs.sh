#!/bin/bash
usbhub --hub D9D1 power state --port 1 $1
usbhub --hub D9D1 power state --port 2 $1
usbhub --hub D9D1 power state --port 3 $1
usbhub --hub D9D1 power state --port 4 $1
usbhub --hub 624C power state --port 1 $1