#!/usr/bin/env bash
set -euo pipefail

# ---- CONFIG ----
BUILD_DIR="${BUILD_DIR:-$(pwd)/build}"
BUILD_TYPE="${BUILD_TYPE:-Debug}"
JOBS="${JOBS:-$(nproc)}"
# Comma list of sanitizers, e.g. SANITIZE=address,undefined ./scripts/build.sh
# Use a separate BUILD_DIR when sanitizing (flags differ from a normal build).
SANITIZE="${SANITIZE:-}"

# ---- Go to the directory that contains this script (project root) ----
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

echo "Project root : $PROJECT_ROOT"
echo "Build dir    : $BUILD_DIR"
echo "Build type   : $BUILD_TYPE"
[ -n "$SANITIZE" ] && echo "Sanitizers   : $SANITIZE"

# ---- Translate SANITIZE into CMake options ----
SANITIZE_ARGS=()
[[ "$SANITIZE" == *"address"*   ]] && SANITIZE_ARGS+=("-DWANKEL_ENABLE_ASAN=ON")
[[ "$SANITIZE" == *"undefined"* ]] && SANITIZE_ARGS+=("-DWANKEL_ENABLE_UBSAN=ON")

# ---- Use Ninja for a fresh configure if available (leave existing build dirs
# on whatever generator they were already configured with) ----
GENERATOR_ARGS=()
if [ ! -f "$BUILD_DIR/CMakeCache.txt" ] && command -v ninja >/dev/null 2>&1; then
    GENERATOR_ARGS+=(-G Ninja)
fi

# ---- Create / enter build dir ----
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# ---- Configure (point CMake at the project root) ----
cmake "$PROJECT_ROOT" \
      -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
      -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
      "${GENERATOR_ARGS[@]}" \
      "${SANITIZE_ARGS[@]}"

# ---- Build ----
cmake --build . --config "$BUILD_TYPE" -j"$JOBS"

echo "Build complete! Binaries are in $BUILD_DIR/bin"
