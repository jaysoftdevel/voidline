#!/usr/bin/env bash
# Optional freedesktop launcher; never writes into the game installation.
set -euo pipefail
root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
bin="$root/run.sh"
[[ -x "$bin" ]] || { echo "Missing Linux editor launcher: $bin" >&2; exit 1; }
application="${XDG_DATA_HOME:-$HOME/.local/share}/applications/org.voidline.ModelStudio.desktop"
mkdir -p -- "$(dirname -- "$application")"
# Escaping quoted paths is required for freedesktop Exec entries.
quoted_bin=${bin//\\/\\\\};quoted_bin=${quoted_bin//\"/\\\"}
quoted_project=${root//\\/\\\\};quoted_project=${quoted_project//\"/\\\"}
cat > "$application" <<EOF2
[Desktop Entry]
Version=1.0
Type=Application
Name=VOIDLINE Model Studio
Comment=Offline VOIDLINE level and character animation editor
Exec="$quoted_bin"
Icon=$root/../icons/model-studio.png
Terminal=false
Categories=Development;Graphics;
StartupNotify=true
EOF2
chmod 644 "$application"
echo "Installed application menu entry: $application"
