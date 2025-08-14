#!/bin/bash
set -e

glslc --target-env=vulkan1.2 -O -g -fshader-stage=frag -o src/shaders/fragment.spv src/shaders/fragment.glsl

glslc --target-env=vulkan1.2 -O -g -fshader-stage=vert -o src/shaders/vertex.spv src/shaders/vertex.glsl
