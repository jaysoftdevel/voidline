# VOIDLINE v2.10.26 — Human gait, jump constraints, prologue

This update applies to a **v2.10.25** game source checkout. It modifies renderer/game code and prologue presentation only. All editable model JSON, movement descriptions, level JSON, saves, and existing build artifacts remain untouched.

## Human motion

- Pelvis advances and rolls gently toward the loaded leg; the shoulder girdle counter-rotates. Both movements are linked to the same two-step phase used by hands and feet.
- Upper-body counter-rotation is reduced during crouch, jump and fall, helping the head stay steady.
- Leg rendering constrains each ankle to the actual combined length of its thigh and shin. The heel and boot move with the ankle, so jumping and falling never draw an impossibly stretched shin or detached foot.
- Knee pose edits remain supported within a small bend allowance; no model source values are overwritten.

## Prologue

The opening retains seven text/story beats, but all use the same `orbital` backdrop. One image is preloaded in co-op instead of seven. The time-based pan and per-beat narration/overlays remain. Level completion story art is unchanged.

## Install and build

```bash
unzip game-update-v2.10.26.zip
./game-update-v2.10.26/apply.sh /path/to/voidline-game
cd /path/to/voidline-game
./build.sh            # incremental
# ./build.sh --clean  # optional full rebuild
```

## Verification

- GCC 14.2 Linux Release build passed without compiler warnings in the final compile.
- Five CTest suites passed, including inverse-kinematics leg-length and opposing-girdle-phase tests.
- Headless rendered smoke tests passed for `prologue-0`, `prologue-4`, and `armsrun`.
- Native Windows compilation and a complete manual gameplay playthrough were not performed.
