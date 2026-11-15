# VOIDLINE v2.10.16 — compiler warning cleanup

Five warnings from the stock GCC 14.2 Linux build (with -Wall -Wextra -Wpedantic -Wshadow) were fixed at their source:

- `src/content/level_io.cpp`: avoid shadowing the terrain-point parser with the `structureVertices` JSON loop variable.
- `src/game/relays.cpp` and `src/game/game.hpp`: rename the relay identifier parameter to avoid shadowing `Game::index`.
- `src/render/figures/head.cpp`: explicitly handle Mercer, Cole, and Nyx in the head-gear switch without changing their existing default appearance.

No diagnostic flags were disabled; no model or level authoring data was replaced.

## Build/verification

```bash
./build.sh            # incremental (default), includes tests
./build.sh --clean    # full rebuild when explicitly requested
```

Verified on Linux: clean Release build with 0 warnings, all 3 CTest suites passed; 20 Python editor tests passed. A subsequent normal build reused objects and completed compiling/linking in 0.9 seconds. Windows uses the same fixed source files but a native Windows build was not performed.
