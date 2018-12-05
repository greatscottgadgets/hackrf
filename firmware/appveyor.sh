#!/bin/sh

pwd
echo $PYTHONPATH
/usr/bin/env python -V
/usr/bin/env python -c "import sys; print sys.path"
/usr/bin/env python -m ensurepip
/usr/bin/env python -m pip install pyyaml

cd firmware/libopencm3
make
cd ..
mkdir build-hackrf-one
cd build-hackrf-one
cmake ..
make
