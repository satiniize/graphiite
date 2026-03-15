#!/bin/bash
set -e

for shader in src/shaders/*.frag src/shaders/*.vert; do
    ext="${shader##*.}"
    glslc --target-env=vulkan1.2 -O -g -fshader-stage="$ext" -o "${shader}.spv" "$shader"
done
