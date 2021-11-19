#!/bin/bash
source testing-venv/bin/activate
git submodule init
git submodule update
mkdir firmware/hackrf_usb/build
cd firmware/hackrf_usb/build
cmake ..
make
cd ../../..
deactivate