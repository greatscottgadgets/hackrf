#!/bin/sh

pwd
echo $PYTHONPATH
which pip
/usr/bin/env python -V
/usr/bin/env python -c "import sys; print sys.path"
pip install --install-option="--prefix=c:\cygwin64/usr/lib/python2.7/site-packages" pyyaml
ls /usr/lib/python2.7/site-packages
cd firmware/libopencm3
make
cd ..
mkdir build-hackrf-one
cd build-hackrf-one
cmake ..
make
