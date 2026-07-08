#!/usr/bin/env bash
# Reformat all first-party source in place with clang-format.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$PROJECT_ROOT"

mapfile -t FILES < <(find src/Wankel Sandbox/src -type f \( -name "*.cpp" -o -name "*.h" -o -name "*.hpp" \))

echo "Formatting ${#FILES[@]} files ..."
clang-format -i "${FILES[@]}"
echo "Done."
