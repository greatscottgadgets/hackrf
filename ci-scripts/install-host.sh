#!/bin/bash
mkdir host/build
cd host/build
cmake ..
make
sudo make install
sudo ldconfig
cd ../..