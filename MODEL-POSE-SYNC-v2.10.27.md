# VOIDLINE v2.10.27 — editable pose animation and export folder

## Authoritative inputs

- `models/characters/*.vlcharacter.json`: six playable character rigs, their available poses and five authored XY keyframes per action.
- `models/enemies/*.vlenemy.json`: four articulated humanoid enemies (the other six remain procedural and have no joint-pose tracks).
- `models/movement-descriptions.json`: descriptions, supported states and pose timing metadata.
- `levels/*.vllevel.json`: authored level overrides; unrelated to pose animation.

Model Studio opens the game checkout directly and never reads an editor-private model cache. A pose's five keyframes are saved under `skeleton.poseFrames.<pose>`. The old `skeleton.sequences.<pose>` entry is still the static additive pose offset. The five keyframes are sampled over a normalized 0-to-1 timeline in both the game renderer and the editor. Existing authored static offsets and any already authored keyframes are preserved when the update is installed.

## Edit a movement

1. From the game root on Linux: `./run-model-studio.sh`.
2. Load a character or humanoid enemy and select a pose such as `jump`, `fall`, `land` or `crouch`.
3. Click **Play** to preview it, or use **Frame** to scrub 0–59.
4. Drag joints to edit the **nearest of five keyframes**; the current key number is shown in the inspector. Other frames are smoothly interpolated. For legacy Base/Aim/Run, dragging continues to edit the corresponding original offset layer.
5. Press **Ctrl+S** to save changes straight to the corresponding source JSON. **Copy Movement** / **Paste Movement** includes the static offset and all five animated keyframes.
6. Run `./build.sh` (incremental) to refresh `assets/models/*.actions.json`, build `assets/game.vpak`, and deploy changes to the executable. No manual `Publish Runtime PAK` step is needed for a rebuild. For an already running game, restart it to load the new pack.

The game still applies physics, foot/ground contact, inverse kinematics, and procedural gait on top of the authored per-action keyframe offsets. The editor shows the same authored track and a simplified representation of gait; it is **not** a pixel-for-pixel game renderer.

## What `export/` is for

`export/` is a **disposable offline output directory**, created by **Export Project Sources** in Model Studio. Its contents include:

- `export/assets/models/*.vlrm` / `*.vlem`: baked character/enemy model binaries.
- `export/assets/models/*.actions.json`: generated action catalogs; `*.info.json`: generated metadata.
- `export/src/content/stages.cpp`, `cooperative.cpp`, `characters.hpp`, `skeleton.hpp` and `enemy_motion.hpp`: optional generated C++ source snapshots.

These are **generated copies**, not the model files opened by the editor, and normal `build.sh` does **not** automatically compile the C++ snapshots in `export/src/` or use `export/assets/` as its inputs. A normal game build packages from the real `assets/` directory. You may remove `export/` if you do not need a previous offline export. Do **not** use it as an authoring location because later exports can overwrite it.

**Export Single Model** separately produces a standalone JSON copy at the location you choose; it does not publish it to the game automatically.

## Update and backup behavior

Apply `game-update-v2.10.27/apply.sh /path/to/voidline-game` to v2.10.26. The patch adds keyframes where absent to existing models, with pre-migration originals backed up in `update-backups/v2.10.27/models/`. It does not replace a model with bundled defaults. Running the installer repeatedly is safe. The C++/Python source fixes and build synchronization are installed alongside this migration.
