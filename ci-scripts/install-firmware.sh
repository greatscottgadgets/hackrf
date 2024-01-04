#!/bin/bash
set -e
git submodule init
git submodule update
mkdir firmware/hackrf_usb/build
cd firmware/hackrf_usb/build
cmake ..
make
cd ../../..
