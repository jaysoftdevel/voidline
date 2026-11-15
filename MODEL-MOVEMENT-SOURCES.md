# Model movement inputs — VOIDLINE v2.10.14

**The editable source models are the primary output.** Updating an installed
`assets/game.vpak` is optional; it is no longer the default editor action.

## Input files (tracked in the game source checkout)

- `models/movement-descriptions.json`: the authoring library of **104 distinct
  movement/action states** and their labels, categories, loop information,
  suggested duration, and description.
- `models/characters/0.vlcharacter.json` … `5.vlcharacter.json`: six editable
  player models, with a 14-joint XY offset and a 14-joint depth (Z) offset for
  every movement state.
- `models/enemies/0.vlenemy.json` … `9.vlenemy.json`: ten editable enemy models
  using the same action description format.

Existing `base`, `aim`, and `run` legacy poses remain unchanged and compatible.
No existing authored vectors are reset when missing movements are filled in.
The movement catalog exposes all states for every entity in the editor. The
runtime presently uses the legacy baked `base`/`aim`/`run` layers; additional
states are **editable and preserved**, but they do not automatically acquire
in-game transitions merely by appearing in the movement library. Procedural
non-humanoid enemies do not consume humanoid joint offsets.

## Load, edit, publish — no cache

1. Open `./run-model-studio.sh --open-workspace "$PWD"` from the game **source
   checkout**. The editor reads all JSON from `models/` and `levels/` in that
   checkout; `game.vpak` and any old editor cache are ignored.
2. Edit a model and choose **Save**. Source JSON changes are saved immediately
   to `models/characters/` or `models/enemies/`, with no intermediate draft or
   separate source commit. F5 reloads the selected source file from disk.
3. **Publish Model Sources + Levels** regenerates `assets/models/` binaries,
   metadata and actions from the source JSON and synchronizes saved `levels/`
   to built runtimes. It does not modify an installed runtime PAK by default.
4. `./build.sh` incrementally repacks the game assets for Linux, or use the
   Windows build script for the same shared game sources. Only `--clean` forces
   a full rebuild.

```sh
./run-model-studio.sh --prepare-workspace "$PWD"  # source root only
./run-model-studio.sh --publish-sources "$PWD"    # no runtime PAK update
./build.sh
```

For an optional immediate runtime PAK update, use **Also Publish Runtime PAK…**
or `./run-model-studio.sh --update-workspace "$PWD"`.

A standalone installed `game.vpak` is not an editable workspace; use its
original source checkout. Old private authoring caches are not consulted or
removed. Refer to `MODEL-STUDIO-WORKFLOW.md` for the full migration note.
