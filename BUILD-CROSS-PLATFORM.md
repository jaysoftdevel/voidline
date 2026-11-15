# VOIDLINE v2.10.14 — complete source, shared game content

This archive contains one C++ game source tree and one asset tree. Linux and Windows
build **the same game** from `src/`, `assets/` and `scripts/pack-assets.py`.
The shipped `game.vpak` is generated at build time from `assets/`, excluding license
notices; it contains the same named media and character/enemy models on both systems.
No files in `model-studio/` are included in the Windows game or game asset pack.

## Linux

Prerequisites: CMake, C++20 compiler, Ninja or Make, Python 3, SDL2, OpenGL,
libpng, libwebp, libsndfile, json-c and pthreads. Tk (`python3-tk`) is needed to
run Model Studio's GUI. On Ubuntu/Debian, run `./install_deps.sh` first.

```sh
./build.sh
./run.sh
./run-model-studio.sh   # Linux only; editor is not bundled in the game runtime
```

`./build.sh` is incremental by default (retains the CMake cache, compiled
objects and previously packed assets). Use `./build.sh --clean` to remove the
entire `build/` directory and perform a complete rebuild. `--debug`,
`--verbose`, `--no-tests` and ordinary CMake `-D...` options still work.

Outputs: `build/voidline/voidline`, `build/voidline/assets/game.vpak`,
`build/dist/VOIDLINE/` (tester runtime), and
`build/model-studio/voidline-editor-core` (Linux-only authoring backend).

## Windows (Visual Studio 2022 / 64-bit)

Requirements: Visual Studio 2022 C++ workload and Windows SDK, CMake, Python 3,
and a bootstrapped vcpkg checkout. The game uses the same `src/`, `assets/`,
`vcpkg.json`, and pack script as Linux.

```bat
set "VCPKG_ROOT=C:\dev\vcpkg"
build.bat
```

Outputs: `build-windows\voidline\Release\voidline.exe` and
`build-windows\dist\VOIDLINE\` with `assets\game.vpak` and required runtime DLLs.
The Windows CMake configure explicitly skips `model-studio/`; it needs no Tk,
editor source builds, or Linux development libraries.

**Windows build verification:** a native Windows toolchain and SDK are not available
in the source assembly environment. Linux build/tests and Windows CMake scripts
have been inspected, but Windows compilation and mixed-OS multiplayer require
verification on Windows hardware or a configured Windows VM. Run
`verify-windows.bat` and the CTest suite after building.

## Content parity and deterministic packaging

Both builds run the same Python asset packer on the same checked-in `assets/`
files, with filenames sorted and a fixed binary encoding. Use the following on
both machines to compare the source content list and packed archive SHA-256:

```sh
python3 scripts/check-source-assets.py
```

The output includes the digest for the *would-be* archive; that digest should be
identical for unmodified source trees on both operating systems. Edited runtime packs and clean builds use identical model data once
**Publish models** writes updated baked models to the game source `assets/`.
A source checkout does not need a pre-existing `game.vpak` for Model Studio.
See `MODEL-STUDIO-WORKFLOW.md` for the exact workflow.

## Included model files

- `assets/models/characters.vlrm`, `characters.info.json`, and `characters.actions.json`:
  all six playable character slots from the current clean authoring snapshot.
- `assets/models/enemies.vlem`, `enemies.info.json`, and `enemies.actions.json`:
  all ten enemy archetypes from the current Model Studio v0.0.17 starter definitions.
- `model-studio/starter-project/models/` contains editable JSON for both groups.
- `model-studio/starter-project/levels/` contains 20 campaign and 6 cooperative layouts.

The game loads the baked models and action catalogs at startup from `game.vpak`.
Published level edits live in source `levels/` instead; builds sync those JSON
files beside the executable and into the tester runtime. Deleting a source
level override restores the built-in layout and removes it from subsequent
builds. Only the Linux editor authors these files; **both OS builds load them**.

## Important file/folder handling

- `build/`, `build-windows/`, local authoring caches, Python bytecode and generated
  exports are deliberately not included in this source ZIP.
- `assets/licenses/` contains license notices, shipped separately in runtime folders.
- Do not replace `game.vpak` while the game is running. Restart the game after
  publishing model edits. Multiplayer peers need matching game/model revisions.
