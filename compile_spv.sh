#!/bin/bash
set -e

glslc --target-env=vulkan1.2 -O -g -fshader-stage=frag -o src/shaders/sprite.frag.spv src/shaders/sprite.frag

glslc --target-env=vulkan1.2 -O -g -fshader-stage=frag -o src/shaders/ui_rect.frag.spv src/shaders/ui_rect.frag

glslc --target-env=vulkan1.2 -O -g -fshader-stage=vert -o src/shaders/basic.vert.spv src/shaders/basic.vert
