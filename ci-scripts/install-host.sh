#!/bin/bash
set -e
mkdir host/build
cd host/build
cmake ..
make
cd ../..
