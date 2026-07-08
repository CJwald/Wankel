# sandbox/buildSandbox.sh
#!/usr/bin/env bash
set -euo pipefail

# ---- CONFIG ----
BUILD_DIR="${BUILD_DIR:-$(pwd)/build}"
BUILD_TYPE="${BUILD_TYPE:-Debug}"
JOBS="${JOBS:-$(nproc)}"
# Comma list of sanitizers, e.g. SANITIZE=address,undefined ./build.sh
# Use a separate BUILD_DIR when sanitizing (flags differ from a normal build).
SANITIZE="${SANITIZE:-}"

# ---- Go to the directory that contains this script (Sandbox root) ----
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "Sandbox root : $SCRIPT_DIR"
echo "Build dir    : $BUILD_DIR"
echo "Build type   : $BUILD_TYPE"
[ -n "$SANITIZE" ] && echo "Sanitizers   : $SANITIZE"

# ---- Translate SANITIZE into CMake options ----
SANITIZE_ARGS=()
[[ "$SANITIZE" == *"address"*   ]] && SANITIZE_ARGS+=("-DWANKEL_ENABLE_ASAN=ON")
[[ "$SANITIZE" == *"undefined"* ]] && SANITIZE_ARGS+=("-DWANKEL_ENABLE_UBSAN=ON")

mkdir -p "$BUILD_DIR" && cd "$BUILD_DIR"
cmake "$SCRIPT_DIR" -DCMAKE_BUILD_TYPE="$BUILD_TYPE" "${SANITIZE_ARGS[@]}"
cmake --build . -j"$JOBS"
echo "Done. Run: ./bin/Sandbox"
