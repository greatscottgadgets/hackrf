#!/bin/bash
set -e
git submodule init
git submodule update
cd firmware/hackrf_usb
cmake -DBOARD=PRALINE -B build
cmake --build build
cd ../..
