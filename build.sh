#!/bin/bash
set -e

./spv_shader.sh

# Configure the project with CMake
cmake -S . -B build

# Build the project
cmake --build build

# Run the binary
./build/software-renderer
