#!/usr/bin/env bash
# Configures and builds VOIDLINE with CMake. Output: build/voidline/voidline
set -euo pipefail
root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
command -v cmake >/dev/null || { echo "CMake is unavailable. Run ./install_deps.sh." >&2; exit 1; }
generator=()
command -v ninja >/dev/null && generator=(-G Ninja)
cmake -S "$root" -B "$root/build" "${generator[@]}" -DCMAKE_BUILD_TYPE=Release "$@"
cmake --build "$root/build" --parallel
echo "Built $root/build/voidline/voidline"
