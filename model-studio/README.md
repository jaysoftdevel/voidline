# VOIDLINE Model Studio 0.0.19 — live game-source editor

Model Studio is a standalone **Linux-only** desktop application integrated into
VOIDLINE v2.10.15's source checkout. It uses the **game's actual input files**;
there is no authoring cache, cloned project, or startup import from game.vpak.

## Interface, copy/paste and resizing

The editor opens maximized (F11 toggles maximize), and status prompts are
resizable, scrollable, and use a larger consistent font. Inspector text and the
bottom status line wrap to available width when the main window is resized.

Edit menu / shortcuts (shortcuts work when a text input is **not** focused):

- **Copy Model / Paste Model** — Ctrl+C / Ctrl+V; paste into another model
  of the same type. All editable model fields and movement states are copied,
  but the destination file's slot `id` and `schema` are retained. Save Ctrl+S
  to persist directly to the source JSON in `models/`.
- **Copy Movement / Paste Movement** — Ctrl+Shift+C / Ctrl+Shift+V;
  copy offsets for all 14 joints and their depth vectors from the selected
  movement and paste into the currently selected movement on another model
  (or another pose of the same model). Editable skeletons only.

Clipboard data is held in the normal desktop clipboard, not an editor file or
workspace cache. Save remains subject to external-change protection.

## Start

From the game source root:

```bash
./build.sh
./run-model-studio.sh --open-workspace "$PWD"
```

The selected source root must contain `CMakeLists.txt`, `models/`, `levels/`,
and `assets/`. Selecting a build folder inside that checkout resolves to the
same source root. Standalone installed PAK folders are not editable workspaces.

Opening a workspace only reads existing model JSON, movement descriptors, and
art assets. If compiled-default campaign or co-op level JSONs do not yet
exist, they are generated **inside the game's levels/ directory** on first
open; existing level/model files are never overwritten. Existing private
editor caches are ignored completely.

## Edit and save

- **Characters/enemies:** Edit scalar properties, articulated joints, depth
  offsets, and any of the 104 defined movement sequences. Model inputs live in
  `models/characters/*.vlcharacter.json`, `models/enemies/*.vlenemy.json`, and
  `models/movement-descriptions.json`.
- **Levels:** Move, add, delete, and reshape objects, including polygon corners;
  files are in `levels/*.vllevel.json`.
- **Media:** Replace art directly under `assets/`.
- **New models:** Create prototypes under `models/custom/` and assign them to
  existing supported game slots to use them at runtime.

**Save / Ctrl+S** writes straight to the matching source file in the game
workspace. **F5** reloads the selected file directly from disk. If the file
changed externally after opening, Save refuses to overwrite it until reloaded
(or the conflict is resolved). **Save As** stays inside the workspace; **Export
Single Model** is the way to export an independent JSON copy.

## Publish and rebuild

**Publish Model Sources + Levels** regenerates derived model binaries, metadata
and action catalogs into `assets/models/`, and synchronizes source level files
to built runtimes. Derived assets are backed up before replacement. The editor
writes its offline export output in the **game source's `export/` directory**.
The publish operation does not update runtime game.vpak by default. Choose
**Also Publish Runtime PAK…** if that is specifically needed.

The source model/level JSON inputs are already saved to the checkout when you
press Save. Run `./build.sh` to incrementally rebuild/pack the game, or use
`./build.sh --clean` for a deliberate clean build. The Windows game build uses
the same model and level files; Model Studio itself is not compiled on Windows.

See [../MODEL-STUDIO-WORKFLOW.md](../MODEL-STUDIO-WORKFLOW.md) for exact commands
and how to migrate wanted edits from old caches without losing user data.
