#!/bin/bash

CMAKE='cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON'

CLANG_TIDY='clang-tidy
    -checks=-*,readability-braces-around-statements
    --warnings-as-errors=-*,readability-braces-around-statements
    --fix-errors
    --format-style=file
    -p'

BUILD=build/host
mkdir -p $BUILD
$CMAKE -S host -B $BUILD
make -j4 -C $BUILD
$CLANG_TIDY $BUILD host/libhackrf/src/*.{c,h} host/hackrf-tools/src/*.c

for board in HACKRF_ONE JAWBREAKER RAD1O; do
    BUILD=build/firmware/$BOARD
    mkdir -p $BUILD
    $CMAKE -S firmware/hackrf_usb -B $BUILD
    make -j4 $BUILD
    if [ $BOARD == RAD1O ]; then
        FILES=`ls firmware/{common,hackrf_usb}/*.{c,h} | grep -v rffc5071`
    else
        FILES=`ls firmware/{common,hackrf_usb}/*.{c,h} | grep -v max2871`
    fi
    $CLANG_TIDY $BUILD \
        --extra-arg=-Ifirmware/common \
        --extra-arg=-Ifirmware/libopencm3/include \
        --extra-arg=-I/usr/arm-none-eabi/include \
        $FILES
done
