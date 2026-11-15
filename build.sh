#!/usr/bin/env bash
# Incremental VOIDLINE build. Reuses CMake objects and relinks only changed code.
#
#   ./build.sh                 incremental build, then run the unit tests
#   ./build.sh --clean         delete build/ and rebuild everything
#   ./build.sh --no-tests      skip the unit tests
#   ./build.sh --verbose       print every compiler and linker command line
#   ./build.sh --debug         build without optimization and with debug info
# Any other argument is passed to CMake, for example -DVOIDLINE_LTO=OFF.
set -uo pipefail

root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
build="$root/build"
log="$root/build.log"
run_tests=1
clean=0
verbose=0
type=Release
extra=()
for arg in "$@"; do
  case "$arg" in
    --clean) clean=1 ;;
    --no-tests) run_tests=0 ;;
    --verbose) verbose=1 ;;
    --debug) type=Debug ;;
    *) extra+=("$arg") ;;
  esac
done

if [[ -t 1 ]]; then
  bold=$'\033[1m'; dim=$'\033[2m'; red=$'\033[31m'; green=$'\033[32m'; yellow=$'\033[33m'; cyan=$'\033[36m'; plain=$'\033[0m'
else
  bold=; dim=; red=; green=; yellow=; cyan=; plain=
fi
now() { date +%s.%N; }
elapsed() { awk -v a="$1" -v b="$2" 'BEGIN { printf "%.1f", b - a }'; }
human() { numfmt --to=iec --suffix=B "$1" 2>/dev/null || echo "$1 B"; }
step() { printf '\n%s[%s/7] %s%s\n' "$bold$cyan" "$1" "$2" "$plain"; }
fail() {
  printf '\n%sERROR: %s%s\n' "$red" "$1" "$plain" >&2
  printf '  Full build log: %s\n' "$log" >&2
  printf '  Last 45 build log lines:\n' >&2
  if [[ -f "$log" ]]; then tail -n 45 "$log" | sed 's/^/    /' >&2; fi
  exit 1
}

total_start=$(now)
: > "$log"
printf '%sVOIDLINE build%s\n' "$bold" "$plain"
printf '  Started      %s\n' "$(date '+%Y-%m-%d %H:%M:%S')"
printf '  Host         %s, %s CPU threads, %s\n' "$(uname -sr)" "$(nproc)" "$(uname -m)"
printf '  Source       %s\n' "$root"
printf '  Build type   %s\n' "$type"
printf '  Log file     %s\n' "$log"

command -v cmake >/dev/null || fail "CMake is unavailable. Run ./install_deps.sh."
compiler="${CXX:-g++}"
command -v "$compiler" >/dev/null || fail "C++ compiler '$compiler' is unavailable. Run ./install_deps.sh or set CXX."
generator=()
generator_name="Unix Makefiles"
if command -v ninja >/dev/null; then generator=(-G Ninja); generator_name=Ninja; fi
printf '  Compiler     %s\n' "$("$compiler" --version | head -n1)"
printf '  CMake        %s\n' "$(cmake --version | head -n1)"
printf '  Generator    %s\n' "$generator_name"

step 1 "Preparing build directory"
if (( clean )); then
  if [[ -d "$build" ]]; then
    size=$(du -sb "$build" 2>/dev/null | cut -f1)
    files=$(find "$build" -type f | wc -l)
    rm -rf -- "$build" || fail "Could not remove $build"
    printf '  --clean: removed %s (%s files, %s)\n' "$build" "$files" "$(human "${size:-0}")"
  else
    printf '  --clean: no previous build directory.\n'
  fi
elif [[ -f "$build/CMakeCache.txt" ]]; then
  printf '  Incremental build: keeping CMake cache and compiled objects.\n'
else
  printf '  First build: creating CMake cache and compiled objects.\n'
fi

step 2 "Configuring"
t=$(now)
# CMake cannot change generator in an existing cache: only choose one for first configure.
if [[ -f "$build/CMakeCache.txt" ]]; then generator=(); fi
if ! CXX="$compiler" cmake -S "$root" -B "$build" "${generator[@]}" -DCMAKE_BUILD_TYPE="$type" "${extra[@]}" >>"$log" 2>&1; then
  printf '  Command: cmake -S %q -B %q -DCMAKE_BUILD_TYPE=%q\n' "$root" "$build" "$type" >&2
  fail "Configuration failed. Check missing libraries, CMake generator, or compiler errors above."
fi
sed -n '/VOIDLINE .* configuration/,/Output/p' "$log" | sed 's/^-- /  /' | grep -v '^  $'
configure_time=$(elapsed "$t" "$(now)")
printf '  %sConfigured in %ss%s\n' "$dim" "$configure_time" "$plain"

step 3 "Compiling and linking"
t=$(now)
# Limit parallel compilation by default; unbounded Ninja jobs can exhaust RAM.
cores=$(nproc)
jobs="${VOIDLINE_JOBS:-$((cores < 8 ? cores : 8))}"
[[ "$jobs" =~ ^[1-9][0-9]*$ ]] || fail "VOIDLINE_JOBS must be a positive integer"
printf '  Parallel jobs %s\n' "$jobs"
build_args=(--build "$build" --parallel "$jobs")
(( verbose )) && build_args+=(--verbose)
warnings=0
status=1
while IFS= read -r line; do
  if [[ "$line" == __voidline_build_exit=* ]]; then
    status="${line#__voidline_build_exit=}"
    continue
  fi
  printf '%s\n' "$line" >>"$log"
  case "$line" in
    *warning:*) warnings=$((warnings+1)); printf '  %s%s%s\n' "$yellow" "$line" "$plain" ;;
    *error:*|*"undefined reference"*|*FAILED:*) printf '  %s%s%s\n' "$red" "$line" "$plain" ;;
    \[*Building*) printf '  %s\n' "$line" ;;
    \[*Linking*) printf '  %s%s%s\n' "$bold" "$line" "$plain" ;;
    *) (( verbose )) && printf '  %s%s%s\n' "$dim" "$line" "$plain" ;;
  esac
done < <(cmake "${build_args[@]}" 2>&1; echo "__voidline_build_exit=$?")
compile_time=$(elapsed "$t" "$(now)")
if [[ "$status" != 0 ]]; then
  printf '  Failed command: cmake' >&2
  printf ' %q' "${build_args[@]}" >&2
  printf '\n  Search compiler errors: grep -nE "error:|FAILED:|undefined reference" %q\n' "$log" >&2
  fail "Compilation or linking failed (exit $status)."
fi
printf '  %sBuilt in %ss with %s warning(s)%s\n' "$dim" "$compile_time" "$warnings" "$plain"

step 4 "Verifying artifacts"
binary="$build/voidline/voidline"
[[ -x "$binary" ]] || fail "Expected executable $binary was not produced."
bytes=$(stat -c %s "$binary")
printf '  Executable   %s (%s)\n' "$binary" "$(human "$bytes")"
printf '  Linked libs  %s\n' "$(ldd "$binary" 2>/dev/null | awk '/=>/ {print $1}' | grep -E 'SDL2|webp|png|sndfile|json' | tr '\n' ' ')"
printf '  Version      %s\n' "$("$binary" --version 2>/dev/null)"
assets="$root/assets"
printf '  Assets       %s files, %s (art %s, audio %s)\n' "$(find "$assets" -type f | wc -l)" "$(human "$(du -sb "$assets" | cut -f1)")" \
  "$(find "$assets" -maxdepth 1 -type f | wc -l)" "$(find "$assets/audio" -type f | wc -l)"
sources=$(find "$root/src" -type f \( -name '*.cpp' -o -name '*.hpp' \))
printf '  Sources      %s lines in %s files\n' "$(xargs cat <<<"$sources" | wc -l)" "$(wc -l <<<"$sources")"
for module in "$root"/src/*/; do
  printf '    %-10s %s files\n' "$(basename "$module")" "$(find "$module" -type f \( -name '*.cpp' -o -name '*.hpp' \) | wc -l)"
done
for check in "$assets/cover.webp" "$assets/fonts/font-regular.png" "$assets/audio/menu.ogg"; do
  [[ -f "$check" ]] || fail "Missing required asset $check"
done
printf '  %sAll required assets are present.%s\n' "$green" "$plain"

step 5 "Running unit tests"
tests_summary="skipped"
t=$(now)
if (( run_tests )); then
  test_log="$build/ctest.log"
  ctest_command=(ctest --test-dir "$build" --output-on-failure --output-log "$test_log")
  printf '  Command       '
  printf '%q ' "${ctest_command[@]}"
  printf '\n'
  printf '  CTest log     %s\n' "$test_log"
  # Capture the full CTest report (including failure stdout/stderr).
  if "${ctest_command[@]}" >"$build/ctest-console.log" 2>&1; then
    tests_summary=$(grep -E 'tests passed' "$build/ctest-console.log" | tail -n1 | sed 's/^ *//')
    grep -E 'Test +#|tests passed|Total Test time' "$build/ctest-console.log" | sed 's/^/  /' || true
  else
    test_exit=$?
    printf '\n%sTEST FAILURE%s (CTest exit %s)\n' "$red" "$plain" "$test_exit" >&2
    printf '  Full test output:\n' >&2
    sed 's/^/    /' "$build/ctest-console.log" >&2
    failed="$build/Testing/Temporary/LastTestsFailed.log"
    if [[ -f "$failed" ]]; then
      printf '\n  Failed test IDs and names:\n' >&2
      sed 's/^/    /' "$failed" >&2
      printf '\n  Re-run each failing test with full diagnostics:\n' >&2
      while IFS= read -r entry; do
        name="${entry#*:}"
        [[ -n "$name" ]] || continue
        printf '    ctest --test-dir %q -R %q --verbose --output-on-failure\n' "$build" "^${name}$" >&2
      done < "$failed"
    fi
    printf '\n  Additional diagnostics:\n' >&2
    printf '    CTest output log: %s\n' "$test_log" >&2
    printf '    Detailed test log: %s\n' "$build/Testing/Temporary/LastTest.log" >&2
    printf '    List tests: ctest --test-dir %q -N -V\n' "$build" >&2
    fail "One or more tests failed; resolve them before distributing the game."
  fi
else
  printf '  Skipped (--no-tests).\n'
fi
test_time=$(elapsed "$t" "$(now)")

step 6 "Staging the tester runtime"
t=$(now)
runtime="$build/dist/VOIDLINE"
if bash "$root/scripts/stage-runtime.sh" "$binary" "$runtime" 2>&1 | tee -a "$log"; then
  :
else
  status="$?"
  printf '  Failed stage command: bash %q %q %q\n' "$root/scripts/stage-runtime.sh" "$binary" "$runtime" >&2
  fail "Runtime staging failed (exit $status)."
fi
stage_time=$(elapsed "$t" "$(now)")

step 7 "Summary"
total=$(elapsed "$total_start" "$(now)")
printf '  %-18s %ss\n' "Configure" "$configure_time"
printf '  %-18s %ss\n' "Compile and link" "$compile_time"
printf '  %-18s %ss\n' "Tests" "$test_time"
printf '  %-18s %ss\n' "Runtime staging" "$stage_time"
printf '  %-18s %s\n' "Tester folder" "$runtime"
printf '  %-18s %ss\n' "Total" "$total"
printf '  %-18s %s\n' "Warnings" "$warnings"
printf '  %-18s %s\n' "Tests" "$tests_summary"
printf '\n%sBuild succeeded.%s Run %s./run.sh%s to play.\n' "$green$bold" "$plain" "$bold" "$plain"
