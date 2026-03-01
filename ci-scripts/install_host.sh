#!/bin/bash
set -e
cd host
cmake -B build
cmake --build build
cd ..
