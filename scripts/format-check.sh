#!/usr/bin/env bash
# Check formatting without modifying anything. Exits non-zero (and lists
# offending files) if anything would be reformatted by ./scripts/format.sh
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$PROJECT_ROOT"

mapfile -t FILES < <(find src/Wankel Sandbox/src -type f \( -name "*.cpp" -o -name "*.h" -o -name "*.hpp" \))

FAILED=()
for f in "${FILES[@]}"; do
    if ! diff -q <(clang-format "$f") "$f" >/dev/null; then
        FAILED+=("$f")
    fi
done

if [ "${#FAILED[@]}" -eq 0 ]; then
    echo "All ${#FILES[@]} files are formatted."
    exit 0
fi

echo "${#FAILED[@]} file(s) need formatting (run ./scripts/format.sh):"
printf '  %s\n' "${FAILED[@]}"
exit 1
