#!/bin/sh
find host/libhackrf/src host/hackrf-tools/src firmware/common firmware/hackrf_usb -iname '*.h' -o -iname '*.hpp' -o -iname '*.cpp' -o -iname '*.c' | xargs clang-format-18 -style=file -i
