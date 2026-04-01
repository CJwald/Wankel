# sandbox/buildSandbox.sh
#!/usr/bin/env bash
set -euo pipefail

mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build . -j$(nproc)
echo "Done. Run: ./bin/Triangle"
