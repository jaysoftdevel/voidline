# Model Studio — loading, exporting and publishing models

**Linux only:** the editor is in `model-studio/`. The Linux and Windows game
builds share `assets/` and load the same baked model formats; the Windows build
never compiles the editor.

1. **Start the editor:** From the game source root run `./build.sh` to build
   the game and editor together, **or** run `cd model-studio && ./build.sh && cd ..`
   to build only the editor (no game PAK is needed). Then launch
   `./run-model-studio.sh --open-workspace "$PWD"`. You may also launch the
   editor and choose **File > Open Game Workspace**. Select the **source root**
   containing `CMakeLists.txt` and `assets/`. No `game.vpak` is needed. A built
   installation (`build/voidline`, `build/dist/VOIDLINE`, or a tester runtime
   containing `assets/game.vpak`) is supported too. The editor resolves the
   project structure, imports the assets/models, and keeps editable data in
   `${XDG_DATA_HOME:-~/.local/share}/voidline-model-studio/workspaces/`.
2. **Edit/save:** Select an entry under Characters, Enemies, or Levels. Edit
   poses, movement sequences, geometry, and properties; choose **Save** (Ctrl+S).
   Reopening the same game source preserves these editable changes. Newly
   created characters/enemies must first be **assigned to an existing game slot**
   (currently 6 character and 10 enemy slots) before they can appear in game.
3. **Export:** **Export Single Model** writes a portable editable JSON file
   only; it does *not* change the game. **Export Project Sources** generates
   C++ source and baked models in the authoring project's `export/` folder;
   it also does *not* publish them. For actual integration use **Publish models**.
4. **Integrate:** Choose **Publish models** (previously Update PAK). The editor
   validates and bakes `characters.vlrm`, `enemies.vlem`, the corresponding
   `*.info.json`, and `*.actions.json`, then writes them into the game source
   `assets/models/`, with backups in `update-backups/`. If an installed game is
   open, its `assets/game.vpak` is replaced with a backup. If working from the
   source root, the editor generates a complete pack in its `export/game.vpak`
   and refreshes any discovered built runtimes under `build/`, `build-release/`,
   `build-debug/`, `build-windows/`, or the full package's `linux-runtime/`.
   When no runtime exists, **build the game**:
   `./build.sh` on Linux or `build.bat` on Windows. Both compile the same source
   assets into a playable `game.vpak`. Restart the game after publishing.
5. **Verify:** Run `./build/voidline/voidline --model-report` (Linux) to see
   which archive is loaded and the baked model offsets. Editing level layouts
   or hard-coded rules that require changed compiled stage C++ still requires
   installing the generated files from `export/src/content/` deliberately and
   rebuilding; publishing models alone does not rewrite gameplay code.

**CLI alternative:**

```sh
./run-model-studio.sh --prepare-workspace "$PWD"
./run-model-studio.sh --update-workspace "$PWD"
./run-model-studio.sh --resolve-workspace "$PWD"
```

For a standalone installed game, a matching source checkout is needed if the
edits must survive future source rebuilds. The editor can publish to a runtime
without it, but warns about that limitation. Do not publish while the game is
running. If you change the models, co-op peers should use matching game assets.
