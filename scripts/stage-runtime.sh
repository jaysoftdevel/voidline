#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 2 ]]; then
  printf 'Usage: %s EXECUTABLE DESTINATION\n' "$0" >&2
  exit 2
fi
root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd -P)"
binary="$1"
destination="$2"
mkdir -p -- "$(dirname -- "$destination")"
stage="$(mktemp -d -- "${destination}.XXXXXX")"
trap 'rm -rf -- "$stage"' EXIT
mkdir -- "$stage/lib" "$stage/licenses"
install -m 755 -- "$binary" "$stage/voidline"
cp -RL -- "$root/assets" "$stage/assets"
cp -- /usr/share/common-licenses/{LGPL-2.1,LGPL-3,GPL-3} "$stage/licenses/" 2>/dev/null || true

# Keep game and codec libraries local. The desktop stack and drivers come from the host.
if ! resolved="$(LD_LIBRARY_PATH= LC_ALL=C ldd "$binary")" || [[ "$resolved" == *'not found'* ]]; then
  printf 'Could not resolve the runtime libraries for %s\n%s\n' "$binary" "$resolved" >&2
  exit 1
fi
while read -r name arrow library _; do
  [[ "$arrow" == '=>' && "$library" == /* ]] || continue
  case "$name" in
    libSDL2-2.0.so.*) packages=(libsdl2-2.0-0) ;;
    libwebp.so.*) packages=(libwebp7) ;;
    libsharpyuv.so.*) packages=(libsharpyuv0) ;;
    libpng16.so.*) packages=(libpng16-16t64 libpng16-16) ;;
    libsndfile.so.*) packages=(libsndfile1) ;;
    libjson-c.so.*) packages=(libjson-c5) ;;
    libFLAC.so.*) packages=("libflac${name##*.}") ;;
    libvorbis.so.*) packages=(libvorbis0a) ;;
    libvorbisenc.so.*) packages=(libvorbisenc2) ;;
    libopus.so.*) packages=(libopus0) ;;
    libogg.so.*) packages=(libogg0) ;;
    libmpg123.so.*) packages=(libmpg123-0t64 libmpg123-0) ;;
    libmp3lame.so.*) packages=(libmp3lame0) ;;
    libz.so.*) packages=(zlib1g) ;;
    libstdc++.so.*) packages=(libstdc++6) ;;
    libgcc_s.so.*) packages=(libgcc-s1) ;;
    *) continue ;;
  esac
  install -m 644 -- "$library" "$stage/lib/$name"
  for package in "${packages[@]}"; do
    copyright="/usr/share/doc/$package/copyright"
    [[ -f "$copyright" ]] || continue
    cp -- "$copyright" "$stage/licenses/$package.txt"
    break
  done
done <<< "$resolved"

cat > "$stage/run.sh" <<'LAUNCH'
#!/bin/sh
set -eu
root="$(CDPATH='' cd -- "$(dirname -- "$0")" && pwd -P)"
export LD_LIBRARY_PATH="$root/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
if [ "${1:-}" = '--check-runtime' ]; then
  dependencies="$(LC_ALL=C ldd "$root/voidline")"
  printf '%s\n' "$dependencies"
  case "$dependencies" in
    *'not found'*) exit 1 ;;
  esac
  shift
  exec "$root/voidline" --version
fi
exec "$root/voidline" "$@"
LAUNCH
chmod 755 "$stage/run.sh"
cat > "$stage/README.txt" <<'README'
Run ./run.sh from this folder. Use ./run.sh --windowed for a window.
Keep the executable, assets and lib directories together.
The system supplies glibc, desktop and audio services, and an OpenGL 3.3 core driver.
Use ./run.sh --check-runtime to show the linked libraries and see missing dependencies.
README

bytes() { find "$@" -type f -printf '%s\n' | awk '{ bytes+=$1 } END { printf "%.0f",bytes }'; }
summary() { printf '%-19s %10s bytes (%s)\n' "$1" "$2" "$(numfmt --to=iec-i --suffix=B "$2")"; }
{
  "$stage/run.sh" --version
  summary Executable "$(bytes "$stage/voidline")"
  summary Assets "$(bytes "$stage/assets")"
  summary 'Runtime libraries' "$(bytes "$stage/lib")"
  summary 'Payload and notices' "$(bytes "$stage/voidline" "$stage/assets" "$stage/lib" "$stage/licenses" "$stage/run.sh" "$stage/README.txt")"
  printf 'Payload totals exclude this report and filesystem overhead.\n'
} > "$stage/BUILD-SIZES.txt"
"$stage/run.sh" --check-runtime > /dev/null

# Keep the previously staged build available until the replacement is ready.
if [[ -e "$destination" ]]; then
  previous="$(mktemp -d -- "${destination}.old.XXXXXX")"
  mv -T -- "$destination" "$previous/runtime"
  if ! mv -T -- "$stage" "$destination"; then
    mv -T -- "$previous/runtime" "$destination"
    rmdir -- "$previous"
    exit 1
  fi
  rm -rf -- "$previous"
else
  mv -T -- "$stage" "$destination"
fi
cat -- "$destination/BUILD-SIZES.txt"
