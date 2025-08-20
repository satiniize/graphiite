#!/bin/bash
set -e

glslc --target-env=vulkan1.2 -O -g -fshader-stage=frag -o src/shaders/basic.frag.spv src/shaders/basic.frag

glslc --target-env=vulkan1.2 -O -g -fshader-stage=vert -o src/shaders/basic.vert.spv src/shaders/basic.vert
