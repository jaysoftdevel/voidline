# VOIDLINE v2.10.18 — progression test isolation

## Fix

`voidline_tests` previously ran beside `voidline`, in the same runtime folder where the
editor and build publish `levels/*.vllevel.json`. When those authored levels changed,
`tests/test_progression.cpp:154` compared the game's correctly loaded custom stage
pointer to a compiled-in stage pointer. This failed even when gameplay was correct.

CTest now executes `build/test-runner/voidline_tests`, isolated from the game's
published overrides. The existing built-in progression assertions remain unchanged.
A separate regression test writes a temporary level override beside the test
executable, verifies that Game::start uses it, then removes it and verifies that
built-in stage loading resumes. No real game levels are modified by that test.

## Run

    ./build.sh
    ctest --test-dir build --output-on-failure

`./build.sh` remains incremental by default; `--clean` forces a full build.
Edited source levels remain in `levels/` and continue to be staged into
`build/voidline/levels/` for the actual game. Native Windows builds are unaffected
except for the shared CMake test-runner isolation.
