#!/bin/bash
set -e

BUILD_TYPE="${1:-Release}"

echo "Building: $BUILD_TYPE"

./compile_spv.sh

cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
cmake --build build --config "$BUILD_TYPE"
