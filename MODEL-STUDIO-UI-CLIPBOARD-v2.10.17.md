# VOIDLINE v2.10.17 — Model Studio UI and build failure diagnostics

## Editor

- Starts **maximized** under the Linux window manager, with F11 to toggle.
- Status, warning, error, confirmation and input prompts now use an adjustable
  Tk dialog with a **12pt DejaVu Sans** font, word wrapping, and a scrollable
  report area. Dialogs are resizable, and initial sizes follow message length
  while respecting screen dimensions.
- The property inspector and bottom status line re-wrap when the editor window
  changes size; long publish reports are scrollable.
- The **Edit** menu has **Copy Model**, **Paste Model**, **Copy Movement** and
  **Paste Movement**, plus their shortcuts.

| Action | Shortcut | Operation |
|---|---|---|
| Copy Model | Ctrl+C | Copy the currently open character or enemy and all its poses |
| Paste Model | Ctrl+V | Paste into an existing model **of the same type**; keep its destination `id` and `schema` |
| Copy Movement | Ctrl+Shift+C | Copy the selected pose's 14 XY and depth vectors |
| Paste Movement | Ctrl+Shift+V | Paste those vectors into the selected pose of the target model |

Keyboard shortcuts deliberately defer to normal system clipboard behavior when
text input controls have focus. Copy/paste only changes the current in-memory
editor document; **Ctrl+S** writes it to the original game source model JSON,
with the existing external-edit conflict check. No authoring cache is used.
Publish continues to build game-input assets from saved model JSON; editing
`game.vpak` is not necessary.

## Build and test diagnostics

```bash
./build.sh              # incremental; runs CTest, including Python editor tests
./build.sh --clean      # full rebuild and CTest
./build.sh --verbose    # show compiler commands
ctest --test-dir build -N -V  # inspect all test commands
```

When configuration, compiling/linking, tests, or runtime staging fails,
`build.sh` exits nonzero with a failure heading, recent build log entries,
and relevant commands. Test failures additionally show complete CTest failure
output, failed test names, verbose single-test rerun commands and paths to:

- `build/ctest-console.log` — captured CTest console output
- `build/ctest.log` — detailed test session
- `build/Testing/Temporary/LastTest.log` — CTest execution details
- `build/Testing/Temporary/LastTestsFailed.log` — failed suite IDs
- `build.log` — complete configure/compile/staging log

**Verified:** Linux Release build, 0 compiler warnings, 4 CTest suites (including
25 Python editor tests) passed; maximized Linux GUI, dialogs, model/movement
clipboard flow smoke-tested with a virtual X display. A synthetic failing CTest
run returned nonzero and printed the failed-test diagnostics. Real CTest passed
again afterward. Windows game source builds remain supported but native Windows
compilation was not run here; the Linux-only editor is excluded on Windows.
