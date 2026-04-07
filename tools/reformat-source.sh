#!/bin/bash

REQUIRED_VERSION=14

VERSION=`clang-format --version | grep -o 'version [^ ]' | cut -d ' ' -f 2 | cut -d '.' -f 1`
if [ "$VERSION" -eq "$REQUIRED_VERSION" ]; then
    CLANG_FORMAT=clang-format
elif command -v clang-format-$REQUIRED_VERSION > /dev/null; then
    CLANG_FORMAT=clang-format-$REQUIRED_VERSION
else
    echo "clang-format version $REQUIRED_VERSION is required."
    exit 1
fi

$CLANG_FORMAT \
    -i \
    --style=file \
    host/libhackrf/src/*.{c,h} \
    host/hackrf-tools/src/*.c \
    firmware/{common,common/rad1o,hackrf_usb}/*.{c,h}
