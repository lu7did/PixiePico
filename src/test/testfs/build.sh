#!/bin/sh
VERSION="1.2"
LIBPATH="/Users/PCOLLA/Documents/GitHub/ADX-ddsPIO/src/ADX-ddsPIO_V"$VERSION
export PICO_SDK_PATH=/Users/PCOLLA/Documents/GitHub/pico/pico-sdk
rm -rf build
mkdir -p build
cd build
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ..
cmake --build . -j
