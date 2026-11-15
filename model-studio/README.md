# VOIDLINE Model Studio v0.0.16 — source-aware game workspaces

**Fix:** Use **Open Game Workspace** to select the VOIDLINE source root, even
before the first CMake build. The editor detects `CMakeLists.txt`,
`scripts/pack-assets.py`, and `assets/`; it reads the source model binaries and
media without needing `assets/game.vpak`. Installed games continue to import
from `assets/game.vpak`. Both modes preserve the editable cache when reopened.

**Publish models** writes newly baked model rigs, metadata, and action layers
to the game source assets and refreshes any built runtime PAKs. If no binary has
been built, simply build the game after publishing. `Export Single Model` is a
JSON copy; `Export Project Sources` produces offline C++ and binary output only.
See `../MODEL-STUDIO-WORKFLOW.md` for the short integration guide.

---

# VOIDLINE Model Studio v0.0.15 — Linux authoring workspace

**Build:** `./build.sh`  ·  **Launch:** `./run.sh`  ·  **Integrated game build:** `./run-model-studio.sh` from the VOIDLINE source root (Linux only).

## New in 0.0.15

- **Movement sequence catalog:** edit 18 supplementary actions where applicable: idle, walk, sprint, crouch, jump, fall, land, fire, reload, hit, death, patrol, chase, alert, recoil, stagger, stairs, and interact. The existing Base/Aim/Run layers remain separately editable. Each humanoid action stores offsets for 14 joints; the game chooses actions from live movement, weapon, landing and damage states and blends them with the procedural gait.
- **Rig-aware controls:** playable humans and humanoid soldiers support joint handles; nonhumanoid drones, turrets, stalkers, scavengers and guardians expose relevant action previews but keep their nonhuman procedural rigs. Their human joint offsets cannot directly modify their existing game renderers.
- **Create new character/enemy:** File > New Character / New Enemy clones a template into `models/custom/`. Assign to Game Slot backs up and replaces one of the six playable character slots or ten enemy slots. Extra playable slots are **not** yet supported by the game menu/network catalog.
- **Structure corners:** select ground, ledge, cover or relay geometry, enable Edit Vertices, then drag corners; toolbar + Point / − Point add and remove vertices (3–32 per polygon). The game reads exported polygons and renders the outlines and surfaces. **Solid collision remains rectangular** until polygon collisions are implemented separately.
- **Responsive Linux interface:** reflowing two-row toolbars, framed selection indicators, wrapping labels, scrollable properties, stretchable fields and wider movement sequence selection.
- **Export and import:** Update PAK writes `characters.actions.json` and `enemies.actions.json` beside baked model binaries and metadata in the game archive, refreshes installed/source archives, and retains backups. Importing a newly opened game archive restores those action layers to the editable JSON models without overwriting earlier authoring edits.

The running game needs **VOIDLINE v2.10.11** for the additional actions and polygon rendering. The editor's own version is independent of the game. Game action offsets use XY: Z vectors remain editable but the present 2D renderer does not apply them. The editor's animations remain procedural previews rather than a guaranteed pixel-perfect representation of the final game.

## First-time Linux setup

```bash
./build.sh
./run.sh
```

Open the game source root or an installed game's folder containing `assets/game.vpak` from the File menu, edit a model or level, Save, then choose **Publish models**. To bring structure geometry or level rules into compiled stages, export source and rebuild the game; merely updating the PAK does not replace level-collision C++ rules.

---

# VOIDLINE Model Studio 0.0.14 — Real-speed animation preview & level editing

The standalone Linux editor now supports an end-to-end edit → publish → rebuild workflow.

**The v0.0.11 bug:** Update PAK wrote to the installed `assets/game.vpak`, but rebuilding the game recreated that package from the source `assets/`, reverting the edits. Version 0.0.12 updates both the installed package and the source assets used by CMake, with file backups. This includes `models/characters.vlrm`, `models/enemies.vlem` and modified artwork.


## New in 0.0.14 — Timeline speed, supported rigs, and level-canvas controls

The Run preview now follows the actual VOIDLINE movement-distance formula for the current model, instead of assigning one arbitrary gait cycle to each 60-frame preview loop. **100%** is the game's sprint gait for playable characters (speed × 1.28, adjusted for stride and walkCycle); humanoid enemy previews use their game distance-per-cycle ratio. The **5–200%** slider is a *preview playback multiplier only*: 100% is normal, and it does not silently modify exported model speed. The timeline advances at 30 sampled frames/second with both Play and Step using the same sampler; moving ground markers show the corresponding world speed while the editable character stays centered.

**Rig compatibility:** all 14 joints are editable for the six humanoid player characters and humanoid enemy archetypes (Ranger, Sniper, Bulwark, Juggernaut). Drone, Turret, Interceptor, Stalker, Scavenger, and Guardian have procedural game renderers without the corresponding human skeleton; Model Studio shows their animated procedural preview proxy but disables unsupported joint dragging. Movement timing and appropriate model properties stay editable. Previously authored joint values are preserved in files even when that rig is unavailable.

**Level layout canvas:** click an object *anywhere on its shape* to select it, drag to reposition. The toolbar supports every current exported level object class: terrain, ledges, cover, stairs, lifts, enemies, checkpoints, supplies, crates, vents, door/switch relays, spawn and extraction. A selected door can be moved with its associated switch, or its switch can be moved independently. Middle or right mouse drag pans both axes; mouse wheel zooms around the cursor; Shift+wheel scrolls horizontally. The Fit Level and Frame Selection actions are available on the level toolbar. The Delete key removes selected objects, except the required spawn and extraction positions (these can be moved). **Saving/exporting writes changes to the editable project; use Update PAK to publish.**

The interface uses native Tk/ttk framed toolbars, highlighted tree selection, explicit level-editing hints, and a scrollable inspector. Source/API compatibility with 0.0.12 is preserved; no VOIDLINE game update is required for these editor-only improvements.

## Correct editing workflow

1. Install VOIDLINE Game v2.10.9 and Model Studio v0.0.14.
2. In the editor choose **Open Game Workspace** and select the folder containing `assets/game.vpak` (normally the game's staged runtime directory, e.g. `<game source>/build/voidline`).
3. Select **Kade Vale (ID 2)** in **Characters**. Choose Base, Aim or Run; drag joints in a viewport. The editor's four synchronized preview panes use a common timeline.
4. Use **Step** for one frame or **Play** to loop the selected pose animation. The inspector includes the character's name, role, schema and editable statistics, dimensions and motion parameters.
5. **Save** the edited model. **Export Model** only writes an individual, portable JSON copy; it does *not* publish to the game.
6. Choose **Update PAK**. The editor detects the parent game source directory automatically or asks you to select it. It writes the new `game.vpak` and synchronizes modified assets into `<game source>/assets/` for subsequent builds. It prints the Kade model vectors and destination.
7. Rebuild VOIDLINE with `./build.sh` or CMake and run the game from the same build/runtime directory.
8. Verify the values using `<game executable> --model-report` (Game v2.10.9 or later) before starting a new game with Kade Vale.

If source folder detection fails, choose the folder containing `CMakeLists.txt` and `scripts/pack-assets.py`, *not* the build/dist directory. A standalone tester runtime cannot preserve changes across an unrelated source build unless its corresponding source assets are updated.

## Build / run

```bash
./build.sh
./run.sh
```

Requires Linux, a C++20 compiler, CMake, Python 3 with tkinter, and the same system PNG/WebP/JSON runtime libraries required by the backend.

Headless verification:

```bash
./build/voidline-editor-core --detect-source /path/to/game/build/voidline
./build/voidline-editor-core --validate /path/to/authoring-project
./build/voidline-editor-core --update-workspace /path/to/game/build/voidline /path/to/game/source
```

## Limits

The preview shows a simplified articulated model and uses the game's gait timing, not the exact textured in-game art. The game is a procedural 2D renderer. XY offsets and body dimensions affect currently rendered character geometry; Z is saved but is not used by the 2D renderer. Level/enemy gameplay rules beyond baked animation may still require the separately generated source replacements.

---

# VOIDLINE Model Studio 0.0.14 — Linux

Model Studio is an independent desktop authoring tool. **The application now uses native
Tk/ttk widgets** (system window decoration, file dialogs, menus, split panes, tabs, tree
navigator, forms and scroll bars), instead of the old SDL-rendered custom interface.
Only the editable model and level drawing viewports use a Canvas.

## Build and launch

Requirements: a C++20 compiler, CMake, Python 3 with Tk (`python3-tk` on Ubuntu/Debian),
libpng, libwebp, json-c runtime libraries, and optionally Pillow for PNG/WebP previews.
**Qt, SDL2 and a browser are not required for the new editor.**

```bash
./build.sh        # builds the C++ import/export backend and runs its tests
./run.sh          # launches the native-widget desktop GUI
```

A precompiled Linux x86-64 backend is included for use via `./run.sh` without compiling.
The GUI is written in Python 3's built-in Tkinter/ttk toolkit; all authoring and packing
logic remains in the compiled C++ backend. The game remains independent.

## Enemy motion editing (new)

Characters and **all ten enemy archetypes** now share the model workspace:
- 14 selectable/drag-adjustable XYZ joints, in synchronized Front, Right, Top and Orbit 3D viewports.
- Three separate Base, Aim and Run animation layers.
- Timeline with Play, Pause, Rewind and single-frame Step.
- Editable proportions, height, width, gait/stride, weapon positioning, idle breathing,
  acceleration, braking, recoil, aim raise, attack delay and enemy statistics.
- Changes can be saved in `voidline-enemy-v2` JSON (existing `v1` enemy files are importable).

The exported `enemies.vlem` package is versioned and checksum-validated; it holds 10
compact motion profiles and is inserted into `game.vpak` by **Update Workspace Exports**.
**VOIDLINE Game v2.10.8** is required to apply enemy motion data in-game: humanoid enemies
use the cached articulated rig; other archetypes use shared movement cadence and timing.
Z-depth is editable in the editor, but the game is still a 2D renderer; depth edits are
preserved in the package but not displayed as full 3D animation in the game.

## Workspace workflow

1. File → **Open Game Workspace…** and choose the installed game directory containing
   `assets/game.vpak` (or run `./run.sh --open-workspace /path/to/VOIDLINE`).
2. A progress dialog opens while resources are extracted to the private authoring cache:
   `~/.local/share/voidline-model-studio/workspaces/<workspace-id>/`.
3. Choose a level, character, enemy or picture in the native project navigator; edit it
   with the viewport handles and property fields. Save with Ctrl+S.
4. **Export Single Model…** writes just the chosen `.vllevel.json`, `.vlcharacter.json`
   or `.vlenemy.json` document to a location you choose.
5. **Update Workspace Exports** builds both `characters.vlrm` and `enemies.vlem`, validates
   offline game models, and rebuilds the installed `game.vpak`. A timestamped backup is
   retained. **Stop VOIDLINE before updating the asset archive.**

To export all authoring files and C++ offline replacements without modifying an installed
asset bundle, use **File → Export Project Sources** or the CLI command below.
Game level geometry and enemy combat *statistics* still require the generated C++ source
replacements and a game rebuild. Only baked rig and movement timing can be changed by
replacing `game.vpak` in game v2.10.8.

## Headless C++ authoring backend

```bash
./run.sh --prepare-workspace /path/to/VOIDLINE
./run.sh --validate /path/to/authoring-project
./run.sh --export-project /path/to/authoring-project
./run.sh --export-single /path/to/authoring-project enemy 7 ./bulwark.vlenemy.json
./run.sh --update-workspace /path/to/VOIDLINE
```

The C++ engine is shipped as `bin/linux/voidline-editor-core`. The `run.sh` launcher
starts the native-widget GUI unless a headless command is given.

## Game compatibility and multiplayer

The game and the editor have separate versions: **Model Studio 0.0.14 / VOIDLINE 2.10.9**.
Game v2.10.8 loads VLEM motion profiles once at startup and incorporates them in the
multiplayer model hash. Co-op partners should use the same game version and asset pack.

## Checks

`./build.sh` runs C++ file/asset/archive tests and Python skeleton preview tests.
The GUI is Linux-only; game sources for Windows are included separately in the optional
v2.10.8 game update, but Windows compilation is not verified here.

## v0.0.14: publishing and rig editor reliability

- Fixes the `test_enemy_game_distance_cycle` KeyError on legacy enemy definitions missing `motion`.
- All character/enemy text and numerical properties are editable, except structural `id` and `schema`.
- Joint offsets are no longer limited to ±25; editor and game validate up to ±65536 units for numeric safety.
- **Update PAK** publishes models/characters.vlrm, models/enemies.vlem and models/characters.info.json to the selected runtime, matching built executable folders and game source/assets, with backups. Restart the running game to pick up the change; a rebuild is not required.
- For a name change, `characters.info.json` is loaded from game.vpak by VOIDLINE v2.10.10.
- **Export Model** produces an individual JSON copy only; **Update PAK** is the action that publishes to game assets.
