# VOIDLINE 2.0.0

VOIDLINE is a native Linux 2D shooter written in C++20 with five sectors, three operatives, difficulty levels, an armory and a skippable opening cinematic. Destroy the guardian in each sector and reach extraction to turn mission score into upgrade points.

## Build and run

Requires a C++20 compiler, CMake, and the SDL2, libwebp, libpng16, libsndfile and json-c runtime libraries plus an OpenGL 3.3 core driver. Development packages are not needed because the platform ABI is declared in `src/platform.hpp`. On Debian or Ubuntu, `./install_deps.sh` installs everything.

```bash
./build.sh
./run.sh
```

The executable is `build/voidline/voidline`. The build links `assets/` next to it. The game starts in fullscreen. Use `./run.sh --windowed` or F11 to switch window modes. Settings includes High and Performance rendering modes.

Progress lives in `${XDG_DATA_HOME:-$HOME/.local/share}/voidline/campaign.json`. Use `--save-file /path/to/campaign.json` for a custom location. Campaigns, settings, best sector records and armory purchases persist. Field checkpoints are available for retries during the current mission.

## Controls

Input is polled every frame from the full keyboard, mouse and gamepad state, so any number of keys can be held together. Each action has alternate keys. If a keyboard cannot report a particular combination (electrical ghosting is a hardware limit), use another key for one of the actions.

| Action | Keyboard and mouse | Gamepad |
| --- | --- | --- |
| Move | A / D or Left / Right | Left stick or D-pad |
| Jump, then flip in mid-air | Space, W, Up or K | A |
| Sprint (hold) | Shift | Left stick click |
| Crouch (hold) | C, S, Down or Ctrl | B |
| Fire | Left click | Right trigger or shoulder |
| Auto-aim fire | F or J | |
| Aim | Mouse | Right stick |
| Bullet time (hold) | Q, X or right click | Left trigger or shoulder |
| Reload | R | X |
| Weapons | 1 to 4, E or mouse wheel | Y |
| Pause | Esc or P | Start |
| Fullscreen | F11 | |
| Menus | Tab or arrows, then Enter | D-pad, A, B |

The Field Carbine and unlimited-reserve Service Pistol are available at the start. The Breach Shotgun and Arc Rifle unlock after the first and second sectors, respectively. The armory offers three upgrade ranks for each primary weapon's damage, magazine, reload speed and reserves.

## Style

Kills chain into combos that multiply score. Airborne, flipping and bullet-time kills earn bonuses and call-outs. Bullet time drains a focus reserve that refills over time and with every kill, and the world slows, cools and pushes in around the operative.

## Layout

| Path | Contents |
| --- | --- |
| `src/` | Game, renderer, input, audio, storage and engine sources |
| `assets/` | Sector art, portraits, weapon art, fonts, music and effects |
| `tests/` | Headless checks for input chords, flips, combos and determinism |
| `docs/RENDERING.md` | Renderer design and references |

Run the tests with `ctest --test-dir build --output-on-failure` after `./build.sh`.

## Diagnostics

Render a short, deterministic scene through the renderer with an offscreen SDL driver. Scenes are `1` to `5`, `menu`, `create`, `armory`, `intro`, `stand0` to `stand2`, `flip` and `slow`, `walk`, `boss` and `gallery0` to `gallery2`.

```bash
SDL_VIDEODRIVER=offscreen SDL_AUDIODRIVER=dummy \
  ./run.sh --smoke-test --windowed --scene slow --frames 90 --screenshot slow.ppm
```
