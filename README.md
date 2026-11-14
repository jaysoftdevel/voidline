# VOIDLINE 2.6.0

VOIDLINE is a native C++ 2D shooter for Linux and Windows 11 with 20 sectors, three operatives, difficulty levels, an armory and a skippable opening cinematic. Destroy the guardian in each sector and reach extraction to turn mission score into upgrade points.

## Build and run

On Windows 11, see [Windows build and Direct3D 12 setup](docs/WINDOWS.md).

On Linux, run `./run.sh` after building from source. A locally built executable takes precedence over a bundled executable from a previous release.

Building from source requires a C++20 compiler, CMake, and the SDL2, libwebp, libpng16, libsndfile and json-c runtime libraries plus an OpenGL 3.3 core driver. Development packages are not needed because the platform ABI is declared in `src/platform.hpp`. On Debian or Ubuntu, `./install_deps.sh` installs everything.

```bash
./build.sh
./run.sh
```

`./build.sh` always starts from an empty `build/` directory. It prints the host, toolchain, configuration, every compile step, warnings, artifact checks (size, linked libraries, asset counts) and the unit test results, with a per-phase timing summary. The full log is saved to `build.log`. Options: `--verbose` shows every compiler command, `--no-tests` skips the unit tests and `--debug` builds without optimization. Other arguments go to CMake, for example `-DVOIDLINE_LTO=OFF`.

The executable is `build/voidline/voidline`. The build links `assets/` next to it. The game starts in fullscreen. Use `./run.sh --windowed` or F11 to switch window modes. Settings includes High and Performance rendering modes.

Progress lives in `${XDG_DATA_HOME:-$HOME/.local/share}/voidline/campaign.json`. Use `--save-file /path/to/campaign.json` for a custom location. Campaigns, settings, best sector records and armory purchases persist. Field checkpoints are available for retries during the current mission.

## Two-player co-op

1. Both players launch VOIDLINE 2.6.0 and open **Multiplayer**.
2. On the host, choose **Host Game**, select a difficulty, and select **Generate Session Key**. The lobby waits for player 2. Copy the key and share it with your partner.
3. On the other computer, choose **Join Game**, paste the key, and select **Connect**. Once both computers load the mission, they play the shared opening cinematic and deploy together in **Chapter 1, The Descent**.

Use a local network or an IPv4 VPN. The host menu fills in a local IPv4 address. Edit it before generating the key if your partner reaches you through a different interface. The key contains that address and a random session identifier. The connection goes directly to the host on TCP port 38274. A key alone cannot connect through an unreachable address, and the game has no relay or matchmaking server.

Each player controls a separate operative and camera. The host simulates enemies, projectiles, pickups, checkpoints and explosive crates for both players at 120 fixed steps per simulation second. It sends up to 60 world snapshots per real second. The guest interpolates motion for operatives, enemies, lifts and projectiles. When the connection falls behind, the host sends its latest available state once the send queue clears. Mission transitions and retries use the same loading and deployment handshake. Bullet time slows the shared mission. Clear the guardian and bring both operatives to the extraction gate. Either operative going down ends the attempt, and the host can retry from the team's latest checkpoint. The host can advance the same session through the remaining sectors.

The co-op menu leaves the mission running for your partner. Closing the session or disconnecting returns the other player to Multiplayer. Co-op progress lasts for the session and unlocks equipment as the team clears sectors. Solo save slots remain independent.

## Controls

Input is polled every frame from the whole keyboard, mouse and gamepad, so any number of keys can be held together, in any order. Opposite directions resolve to the one pressed last, and a tap shorter than one frame still counts. Every action has up to three keys, and all of them can be remapped under Settings, Controls.

Keyboards can fail to report certain combinations when many keys are held, because of electrical ghosting. No program can detect that. Press **F3** at any time, or open Settings, Controls, to see the keys the computer actually receives and the most that were held together. If a key never appears in that list, the keyboard did not send it. Move one of the actions to a different key, or switch sprint or crouch to toggle mode, which needs one finger fewer.

| Action | Keyboard and mouse (default) | Gamepad |
| --- | --- | --- |
| Move | A / D or Left / Right | Left stick or D-pad |
| Jump, then flip in mid-air | Space, W or Up | A |
| Sprint | Shift (hold, toggle or automatic) | Left stick click |
| Crouch | C, S or Down (hold or toggle) | B |
| Fire | Left click | Right trigger or shoulder |
| Auto-aim fire | F or J | |
| Aim | Mouse | Right stick |
| Bullet time (hold) | Q, X or right click | Left trigger or shoulder |
| Reload | R | X |
| Weapons | 1 to 7, E or mouse wheel | Y |
| Pause | Esc or P | Start |
| Fullscreen | F11 | |
| Input diagnostics | F3 | |
| Menus | Tab or arrows, then Enter | D-pad, A, B |

Esc, F3, F11, Tab and Enter are reserved and cannot be assigned. Jumping from a crouch stands up and jumps in one motion, whichever key was pressed first.

## Weapons and upgrades

| Weapon | Role | Unlock |
| --- | --- | --- |
| Field Carbine | Automatic, long reach | Start |
| Service Pistol | Unlimited-reserve sidearm | Start |
| Breach Shotgun | Six pellets, wide spread | Sector 1 |
| Hornet SMG | Very high rate of fire | Sector 1 |
| Arc Rifle | Fast bolts that pierce a second target | Sector 2 |
| Ember Launcher | Lobbed explosive shells with area damage | Sector 3 |
| Longshot Railgun | Hypersonic slug that pierces four targets | Sector 4 |

Every weapon has eight upgrades with three ranks each: damage, magazine, reload speed, ammo reserve, range, fire rate, accuracy and critical hits. The Operative tab of the armory adds six upgrades that apply to every weapon: suit integrity, a lighter frame (speed and jump), a larger bullet-time reserve, a scavenger bonus for pickups, field medic healing and ballistic plating. The carbine, pistol, shotgun and arc rifle all reach farther than in 2.0. Saves from earlier versions load unchanged.

## Campaign and cover

The sector map shows five missions per page. Clear a sector to unlock the next one. Existing saves keep their progress and unlock Sector 6 when Terminus is already cleared. Routes get longer, with wider gaps, moving lifts, elevated routes, timed vents and elite enemies. Later sectors have multiple field checkpoints and guarded approaches to the extraction arena.

Crouch behind a barricade with **C** to break enemy line of sight. Stand and aim over it to fire. Jump to cross it. Shots hit barricades from either side.

| Explosive container | Projectile hits | Blast radius |
| --- | ---: | ---: |
| Charge box | 2 | 100 |
| Fuel crate | 4 | 155 |
| Reactor cell | 7 | 218 |

Each projectile, including an individual shotgun pellet or an enemy round, removes one lit segment. The larger containers deal more damage. Blast damage falls off with distance and walls reduce the damage behind them. A blast detonates other containers within range and can damage the operative and enemies.

## Living backdrops

Each environment has its own background events that play out behind the action: distant detonations and transports in The Descent, steam vents, geysers and eels in Tideworks, drifting spores and bat flocks in Overgrowth, forge eruptions and lava fountains in the Foundry, and lightning, search lights and shooting stars at Terminus. Blasts and lightning light the scene, shake the ground slightly and are heard in the distance.


## Style

Kills chain into combos that multiply score. Airborne, flipping and bullet-time kills earn bonuses and call-outs. Bullet time drains a focus reserve that refills over time and with every kill, and the world slows, cools and pushes in around the operative.

## Combat feedback

Every hit is shown on the enemy. Bullets leave scorched holes with cracks and a glowing rim that cools, sparks fly from the entry and exit points, and a floating damage number appears (rapid hits are merged). A health bar with a pale trailing segment shows how much each hit took, and the crosshair flashes on hit and turns gold on a kill. Soldiers react by body part: a head shot snaps the head back, a torso hit rocks the spine and a leg shot buckles the knees. Badly hurt machines smoke and spark.

Each enemy type dies differently. Soldiers stagger, buckle, drop the rifle and topple about their feet. Drones lose a rotor, tumble trailing fire and explode on impact. Turrets droop, spark and lose their barrels in a secondary explosion. Stalkers are thrown onto their backs and curl their legs in. Guardians shudder and sink while explosions walk across the hull, then end in one large blast.

## Layout

| Path | Contents |
| --- | --- |
| `src/core/` | Vectors, colors and math helpers |
| `src/content/` | Characters, weapons, upgrades and the 20 sector layouts as data |
| `src/game/` | The simulation, one file per feature: lifecycle, player movement (stance, run, jump, fall, gait, aim), weapons, projectiles, enemy AI, combat, hit reactions, death animations, effects, ambient events, triggers |
| `src/input/` | Actions, key bindings and the polled input state |
| `src/platform/` | SDL and OpenGL declarations and the OpenGL loader |
| `src/services/` | Audio mixing and campaign storage |
| `src/net/` | Direct multiplayer sessions, input and world snapshots |
| `src/render/` | Renderer core, GPU helpers, shaders, post-processing passes, scenery, ambient scenes and the figures (rig, legs, torso, head, arms, per-enemy drawing) |
| `src/ui/` | Application state, widgets and one file per screen |
| `src/engine/` | Window, frame loop and command line options |
| `assets/` | Sector art, portraits, weapon art, fonts, music and effects |
| `tests/` | Headless tests, one file per area: input, movement, combat, hits, progression, world |
| `docs/RENDERING.md` | Renderer design and references |

Each folder under `src/` that has sources becomes one static library (game, services, render, ui) and they link in that order, so lower modules never include higher ones. Run the tests with `ctest --test-dir build --output-on-failure` after `./build.sh`.

## Diagnostics

Render a short, deterministic scene through the renderer with an offscreen SDL driver. Scenes are `1` to `20`, `map`, `menu`, `create`, `armory`, `intro`, `stand0` to `stand6`, `flip` and `slow`, `walk`, `arms`, `armsrun`, `boss`, `controls`, `hit0` to `hit4` (one enemy type shot to death) and `gallery0` to `gallery2`. Add `--follow` to crop the screenshot around the action.

```bash
SDL_VIDEODRIVER=offscreen SDL_AUDIODRIVER=dummy \
  ./run.sh --smoke-test --windowed --scene slow --frames 90 --screenshot slow.ppm
```
