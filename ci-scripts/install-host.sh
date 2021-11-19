#!/bin/bash
source testing-venv/bin/activate
mkdir host/build
cd host/build
cmake ..
make
make install
ldconfig
cd ../..
deactivate