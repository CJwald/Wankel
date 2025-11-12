#!/usr/bin/env bash
set -euo pipefail

# ---- CONFIG ----
BUILD_DIR="${BUILD_DIR:-$(pwd)/build}"
BUILD_TYPE="${BUILD_TYPE:-Debug}"
JOBS="${JOBS:-$(nproc)}"

# ---- Go to the directory that contains this script (project root) ----
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

echo "Project root : $PROJECT_ROOT"
echo "Build dir    : $BUILD_DIR"
echo "Build type   : $BUILD_TYPE"

# ---- Create / enter build dir ----
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# ---- Configure (point CMake at the project root) ----
cmake "$PROJECT_ROOT" \
      -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
      -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# ---- Build ----
cmake --build . --config "$BUILD_TYPE" -j"$JOBS"

echo "Build complete! Binaries are in $BUILD_DIR/bin"
