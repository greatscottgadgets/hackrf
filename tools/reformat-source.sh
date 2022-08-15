#!/bin/bash

VERSION=`clang-format --version | grep -o '[^ ]*$' | cut -d '.' -f 1`
if [ "$VERSION" -ge "14" ]; then
    CLANG_FORMAT=clang-format
elif clang-format-14 --version > /dev/null; then
    CLANG_FORMAT=clang-format-14
else
    echo "clang-format version 14 or higher is required."
    exit 1
fi

$CLANG_FORMAT \
    -i \
    --style=file \
    host/libhackrf/src/*.{c,h} \
    host/hackrf-tools/src/*.c \
    firmware/{common,common/rad1o,hackrf_usb}/*.{c,h}
