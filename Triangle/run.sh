#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${BUILD_DIR:-$(pwd)/build}"
BINARY="$BUILD_DIR/bin/Triangle"

if [[ ! -f "$BINARY" ]]; then
    echo "Triangle binary not found! Run ./scripts/build.sh first."
    exit 1
fi

echo "Launching Triangle ..."
exec "$BINARY" "$@"
