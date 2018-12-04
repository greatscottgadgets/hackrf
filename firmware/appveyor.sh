#!/bin/sh

pip install pyyaml
cd firmware/libopencm3
make
cd ..
mkdir build-hackrf-one
cd build-hackrf-one
cmake ..
make
