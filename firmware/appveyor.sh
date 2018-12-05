#!/bin/sh

/usr/bin/env python -m ensurepip
/usr/bin/env python -m pip install pyyaml

cd firmware/libopencm3
export SRCLIBDIR="c:\projects\hackrf\firmware\libopencm3\lib\""
make V=1 lib/lpc43xx/m0
ls -l /cygdrive/c/projects/hackrf/firmware/libopencm3/lib/
make V=1 lib/lpc43xx/m4
cd ..
mkdir build-hackrf-one
cd build-hackrf-one
cmake -G "Unix Makefiles" ..
make
