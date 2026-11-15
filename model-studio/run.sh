#!/usr/bin/env bash
set -euo pipefail
root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
backend="$root/build/voidline-editor-core"
[[ -x "$backend" ]] || backend="$root/bin/linux/voidline-editor-core"
[[ -x "$backend" ]] || backend="$root/../build/model-studio/voidline-editor-core"
# Preserve the complete existing C++ automation/headless interface.
case "${1:-}" in
 --validate|--import-game|--export-project|--export-all|--export-single|--replace-asset|--prepare-workspace|--publish-sources|--update-workspace|--detect-source|--resolve-workspace|--help)
   [[ -x "$backend" ]] || { echo "Build the backend first using ./build.sh" >&2; exit 1; }
   exec "$backend" "$@";;
esac
exec python3 "$root/ui/model_studio.py" "$@"
