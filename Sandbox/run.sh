#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${BUILD_DIR:-$(pwd)/build}"
BINARY="$BUILD_DIR/bin/Sandbox"

if [[ ! -f "$BINARY" ]]; then
    echo "Sandbox binary not found! Run ./scripts/build.sh first."
    exit 1
fi

echo "Launching Sandbox ..."
exec "$BINARY" "$@"
