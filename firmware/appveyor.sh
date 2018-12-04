#!/bin/sh

pwd
echo $PYTHONPATH
/usr/bin/env python -V
/usr/bin/env python -c "import sys; print sys.path"
pip install --install-option="--prefix=/usr/lib/python2.7/site-packages" pyyaml
# pip2 install pyyaml
ls /usr/lib/python2.7/site-packages
cd firmware/libopencm3
make
cd ..
mkdir build-hackrf-one
cd build-hackrf-one
cmake ..
make
