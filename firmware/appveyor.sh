#!/bin/sh

pwd
echo $PYTHONPATH
which pip
easy_install-2.7 pip
/usr/bin/env python -V
/usr/bin/env python -c "import sys; print sys.path"
/usr/bin/env python -m pip install --install-option="--prefix=/usr/lib/python2.7/site-packages" pyyaml
ls /usr/lib/python2.7/site-packages
cd firmware/libopencm3
make
cd ..
mkdir build-hackrf-one
cd build-hackrf-one
cmake ..
make
