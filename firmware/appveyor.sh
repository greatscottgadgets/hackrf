#!/bin/sh

/usr/bin/env python -m ensurepip
/usr/bin/env python -m pip install pyyaml

cd firmware/libopencm3
make lib/lpc43xx/m0
make lib/lpc43xx/m4
cd ..
mkdir build-hackrf-one
cd build-hackrf-one
cmake ..
make
