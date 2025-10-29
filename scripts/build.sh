#!/usr/bin/env bash
set -euo pipefail

# -------------------------------------------------
# Configurable variables
# -------------------------------------------------
BUILD_DIR="${BUILD_DIR:-$(pwd)/build}"
BUILD_TYPE="${BUILD_TYPE:-Debug}"
NUM_JOBS="${NUM_JOBS:-$(nproc)}"

# -------------------------------------------------
# Helper
# -------------------------------------------------
print() { echo -e "\033[1;34m==>\033[0m $*\n"; }

# -------------------------------------------------
# Create / enter build directory
# -------------------------------------------------
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

print "Configuring CMake (type: $BUILD_TYPE) ..."
cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
      -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
      ..

print "Building (jobs: $NUM_JOBS) ..."
cmake --build . --config $BUILD_TYPE -j$NUM_JOBS

print "Build finished! Binaries are in $BUILD_DIR/bin"
