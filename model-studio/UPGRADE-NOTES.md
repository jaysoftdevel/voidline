## 0.0.18 / VOIDLINE 2.10.15 — source-only workspace

Model Studio no longer imports or reuses any private authoring cache. The game
checkout is the active editor project; Save commits edits directly to its input
files. Old caches are preserved but ignored. Pick a source root, not an
installed PAK. F5 reloads current file from disk. Publish bakes source assets;
rebuilds remain incremental. See `../MODEL-STUDIO-WORKFLOW.md`.

---

# Model Studio v0.0.17 upgrade notes

- Opening a VOIDLINE **source checkout** no longer requires `game.vpak`; the
  game assets, models, and levels are auto-resolved from the source structure.
- Installed/runtime game folders continue using their packaged `game.vpak`.
- Publish models updates authoritative source assets and any discovered built
  archives. A source-only workspace generates its archive in the authoring
  cache and supports the very first game build with no existing runtime.

# Model Studio v0.0.15 upgrade notes

From v0.0.14, apply the incremental package to the existing source checkout or use the complete source package. Run `./build.sh` and `./run.sh`. Existing authoring projects are preserved; source backups are placed in `update-backups/`.

The game must be updated to v2.10.11 for supplemental action sequences and cosmetic polygon layers. On Linux the game can build Model Studio automatically from `model-studio/`; on Windows that directory is ignored completely by CMake. New character and enemy prototypes must be assigned to one of the currently supported game slots before publishing. Polygon vertices are visual overlays; collisions remain based on the original rectangles.

---

# Model Studio v0.0.14 upgrade notes

From v0.0.12: use the incremental `apply.sh` to replace the editor code, then run `./build.sh` and `./run.sh`. Existing projects, workspace caches, edited model JSON, game media and baked packages are preserved.

- Run animation matches VOIDLINE movement-distance gait timing at 100%, with a 5–200% **preview-only** slider and one-frame Step.
- Joint editing is enabled only for character and humanoid-enemy rigs used by the game's renderer. Nonhumanoid enemy previews are explicitly procedural proxies, not falsely editable human skeletons.
- Level workspace supports click-on-shape selection, mouse drag movement, wheel zoom, both-axis middle/right drag pan, every exportable object class, and Delete for selected nonessential objects.
- Clearer grouped toolbars, headings, selection highlights and tool hints.

No game executable update is necessary for this release. A new render bake is only required when you change and publish actual models.

# Model Studio 0.0.11 → 0.0.12

- Publishing now mirrors the changed baked models and other modified assets to the game CMake **source** asset tree, as well as the installed game.vpak. This avoids losing edits after rebuilding.
- The editor shows the exact game source destination and verifies Kade Vale ID 2 offsets in the exported model.
- Enhanced Play/Step previews with an animated Run cycle, aim/recoil sequence and idle motion; frame counter updates continuously.
- The native Tk tree uses 30-pixel rows (fixes overlapping files on many Linux desktop themes).
- The scrollable inspector shows schema, character name and role as non-editable metadata above all numeric/appearance settings.
- Export Model creates a single JSON copy. Update PAK is the publish action.
- Existing authoring caches and source assets are preserved; modified source files are backed up under source/update-backups/.

Model Studio 0.0.14 fixes the legacy enemy timing test and adds direct PAK publishing of all detected runtime copies. Match with game 2.10.10 to apply edited character names directly.
