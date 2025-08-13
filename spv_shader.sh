#!/bin/bash
set -e

glslc -fshader-stage=vertex src/shaders/vertex.glsl -o src/shaders/vertex.spv

glslc -fshader-stage=fragment src/shaders/fragment.glsl -o src/shaders/fragment.spv
