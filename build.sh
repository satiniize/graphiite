#!/bin/bash
set -e

# Default build type
BUILD_TYPE="Release" # <-- Set default to Release

# Check for an argument to override the build type
if [ -n "$1" ]; then # Checks if the first argument ($1) is non-empty
    BUILD_TYPE="$1"
fi

# Convert to title case for CMake compatibility (e.g., "debug" -> "Debug")
BUILD_TYPE=$(echo "$BUILD_TYPE" | awk '{print toupper(substr($0,1,1))tolower(substr($0,2))}')

echo "Building with configuration: $BUILD_TYPE"

# Run your shader script (if it's independent of build type)
./spv_shader.sh

# Configure the project with CMake
# The -D flag sets a CMake variable, overriding the default
cmake -S . -B build -DCMAKE_BUILD_TYPE="$BUILD_TYPE"

# Build the project
# --config flag ensures we build the specific configuration
cmake --build build --config "$BUILD_TYPE"

# # Run the binary (commented out as Zed will handle this)
# ./build/"$BUILD_TYPE"/software-renderer
