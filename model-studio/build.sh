#!/usr/bin/env bash
set -euo pipefail
root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
cd "$root"
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel "${JOBS:-2}"
ctest --test-dir build --output-on-failure
python3 -m unittest discover -s ui -p 'test_*.py' -v
printf '
Model Studio v0.0.16 ready. Start with ./run.sh
'
