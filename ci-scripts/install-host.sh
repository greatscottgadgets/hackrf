#!/bin/bash
mkdir host/build
cd host/build
cmake ..
make
make install
ldconfig
cd ../..