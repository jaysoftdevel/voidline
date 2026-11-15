# VOIDLINE Model Studio — live source workspace (v2.10.15)

Model Studio is Linux-only. **No authoring cache is created or used.** Always open
an editable game source checkout with `CMakeLists.txt`, `assets/`, `models/`, and
`levels/`. Selecting a directory inside that checkout resolves to its source
root, including a built runtime inside `build/`. A separately installed
`game.vpak` cannot serve as an editable model workspace; select its original
source checkout instead.

## Open, edit, and rebuild

```sh
./build.sh                                # incremental Linux game + editor build
./run-model-studio.sh --open-workspace "$PWD"
# For CLI validation / publishing:
./run-model-studio.sh --prepare-workspace "$PWD"
./run-model-studio.sh --publish-sources "$PWD"
./build.sh                                # rebuilds game.vpak from source assets
```

On first opening an older source checkout, the editor writes any *missing*
compiled-default campaign/co-op level definitions into that checkout's
`levels/` directory so every editable level has a source file. Existing level
files and existing `models/` files are **never overwritten during open**.
Missing editable models or `models/movement-descriptions.json` cause a clear
error rather than silently substituting private copies.

**Save (Ctrl+S)** writes the currently edited character/enemy/level JSON
*directly* into the game source (`models/characters/`, `models/enemies/`,
`models/custom/`, `levels/`). Editing images writes to `assets/`. There is no
separate publish/commit step for these input files. If the file changed on disk
since it was opened, Save refuses to replace it: choose **View → Reload Selected
File from Game Workspace** (F5) after resolving the conflict. Reopening or
switching to a file reads it again from disk, not from a saved editor cache.
**Export Single Model** creates a separate portable JSON copy; Save As stays
inside the game workspace.

**Publish Model Sources + Levels** regenerates `assets/models/*.vlrm`,
`*.vlem`, `*.info.json` and `*.actions.json` from the live `models/` inputs.
It also synchronizes live `levels/` files to existing game runtimes. It does
not touch runtime `game.vpak` by default. **Also Publish Runtime PAK** rebuilds
runtime packs explicitly if required. **Export Project Sources** writes
standalone generated sources and model artifacts under the **game checkout's**
`export/` folder, not a private editor location.

Run `./build.sh` (incremental; `--clean` only for full rebuild) after edits.
Linux and Windows builds consume the same source assets and levels, but the
Windows build excludes Model Studio. Existing private caches in
`~/.local/share/voidline-model-studio/workspaces/` are ignored. The editor
neither reads nor migrates them; manually copy any wanted legacy changes into
`models/` or `levels/` *before* opening the workspace, then archive/delete the
old cache manually if desired. **The update never deletes user data.**
