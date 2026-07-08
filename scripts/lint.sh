#!/usr/bin/env bash
# Run clang-tidy (via run-clang-tidy) over first-party source only, using
# an existing build's compile_commands.json. Run ./scripts/build.sh first.
#
# Usage:
#   ./scripts/lint.sh                 # report findings
#   ./scripts/lint.sh -fix            # apply auto-fixes where clang-tidy can
#   BUILD_DIR=build ./scripts/lint.sh # point at a specific build dir
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$PROJECT_ROOT"

BUILD_DIR="${BUILD_DIR:-$PROJECT_ROOT/build}"
JOBS="${JOBS:-$(nproc)}"

if [ ! -f "$BUILD_DIR/compile_commands.json" ]; then
    echo "No compile_commands.json in $BUILD_DIR - run ./scripts/build.sh first."
    exit 1
fi

echo "Linting first-party sources against $BUILD_DIR/compile_commands.json ..."
run-clang-tidy -p "$BUILD_DIR" -j "$JOBS" -quiet "$@" '(src/Wankel|Sandbox/src)/.*\.(cpp|h|hpp)$'
