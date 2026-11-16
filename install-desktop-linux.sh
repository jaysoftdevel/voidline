#!/usr/bin/env bash
# Register a launcher for a source checkout; icon remains local to the checkout.
set -euo pipefail
root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
[[ -x "$root/run.sh" ]] || { echo "Missing game launcher" >&2; exit 1; }
file="${XDG_DATA_HOME:-$HOME/.local/share}/applications/org.voidline.Game.desktop"
mkdir -p -- "$(dirname -- "$file")"
quoted="${root//\\/\\\\}"
quoted="${quoted//\"/\\\"}"
cat > "$file" <<EOF
[Desktop Entry]
Version=1.0
Type=Application
Name=VOIDLINE
Comment=Standalone sci-fi action game
Exec="$quoted/run.sh"
Path=$root
Icon=$root/icons/voidline.png
Terminal=false
Categories=Game;ActionGame;
StartupWMClass=VOIDLINE
StartupNotify=true
EOF
chmod 644 "$file"
echo "Installed $file"
