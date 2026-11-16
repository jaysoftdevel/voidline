# VOIDLINE v2.10.25 — Human running and automatic workspace loading

## Run and sprint gait

The C++ locomotion code now uses a continuous left/right stride cycle instead
of a simple sine-wave foot swing. The two feet are half a cycle out of phase.
Each stride passes through heel strike and loading, fixed planted support,
heel-to-toe roll, toe-off, a short double-flight interval, folded-leg recovery,
forward knee drive and the next heel strike. Feet are anchored in world space
while planted, then lifted through an eased recovery arc. The body compresses on
landing and rises in flight; arms move opposite their corresponding legs.

Players and humanoid enemies use the same pure running-cycle sample routine,
with different stance timings and stride lengths. Animation speed tracks actual
horizontal travel rather than screen refresh or wall-clock time. Movement
physics (collision, platforms, jump momentum and controls) are unchanged.

The Linux Model Studio preview now depicts this same phased running cycle using
articulated knee positions instead of symmetric sine oscillations. Authored
model joint offsets are still applied on top of the procedural motion; the
source `models/` JSON files are not modified by this update.

## Automatic game source loading

Launch from the game root:

```bash
./run-model-studio.sh
```

The editor locates its own game source checkout and opens it on startup, even
if invoked from another working directory. It does not use an editor cache or
persist a last-opened project. To explicitly select a different checkout:

```bash
./run-model-studio.sh --open-workspace /path/to/voidline-game
```

## Build

```bash
./build.sh           # incremental, tests enabled
./build.sh --clean   # full rebuild
```

No authored models, movement descriptions or level JSON files are overwritten.
The patch backs up changed code and documentation to
`update-backups/v2.10.25/`. Linux and Windows share the C++ gameplay/render
source; the Python Model Studio is Linux-only.
