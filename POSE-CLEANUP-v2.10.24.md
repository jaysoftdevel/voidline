# VOIDLINE v2.10.24 — per-model runtime pose cleanup

This patch applies to **v2.10.23** and includes only modified game/editor source plus a one-time source-model migration. It does not replace level files or the entire game source tree.

## Pose selection after migration

| Model | Editor pose list |
|---|---|
| Characters 0–5 | `base`, `aim`, `run`, `idle`, `walk`, `sprint`, `crouch`, `jump`, `fall`, `land`, `fire`, `reload`, `hit`, `death`, `alert`, `stairs` |
| Humanoid enemies 1, 6, 7, 9 (Ranger, Sniper, Bulwark, Juggernaut) | `base`, `aim`, `run`, `idle`, `patrol`, `stairs`, `alert`, `fire`, `recoil`, `hit`, `stagger`, `death` |
| Procedural enemies 0, 2, 3, 4, 5, 8 (Drone, Turret, Stalker, Guardian, Scavenger, Interceptor) | No articulated poses (parameters remain editable) |

The `base`, `aim` and `run` internal rig layers are retained in JSON for binary compatibility even for procedural enemies, but are not shown for procedural models: their renderer does not consume humanoid joints. No enemy-specific procedural animation system was removed.

`models/movement-descriptions.json` now defines the **16 unique runtime-selected action names**. The previous 104-state list contained states not selected by the current runtime. The unreachable `chase` branch and unused `interact` runtime action were removed. The game keeps procedural movement and its existing gameplay unchanged.

## Source-first migration and safety

Apply using the provided `apply.sh`; it trims **only built-in models** under `models/characters` and `models/enemies`, and the bundled `model-studio/starter-project` examples. **Your existing supported pose offsets, depth values, model stats, names and custom models are preserved**. Unused action data is backed up under `update-backups/v2.10.24/`. You can recover a removed pose from the backed-up JSON if needed.

Migration also regenerates `assets/models/{characters,enemies}.actions.json` from the trimmed **source model files**. It does not edit `game.vpak`. The next incremental `./build.sh` repacks the generated assets. The editor's subsequent Publish operation also exports only poses that exist in each source model; it will not resurrect removed entries.

The editor builds its pose list on **each file load** from the selected model's own `skeleton.sequences` and `sequenceDepth`, plus the applicable legacy rig layers. F5 reloads the selected source file. You can still author custom poses by adding them to the source model; they are shown only for that model and are not activated in gameplay unless runtime selection is implemented.

## Install

```bash
unzip game-update-v2.10.24.zip
./game-update-v2.10.24/apply.sh /path/to/voidline-game
cd /path/to/voidline-game
./build.sh
./run-model-studio.sh --open-workspace "$PWD"
```

Build remains incremental by default; only `--clean` requests a clean rebuild. Native Windows compilation was not performed.
