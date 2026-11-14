# VOIDLINE 2.10.4

VOIDLINE is a native C++ 2D shooter for Linux and Windows 11 with 20 sectors, six operatives, difficulty levels and an armory. Destroy the guardian in each sector and reach extraction to turn mission score into upgrade points and see the next part of the Vega-9 rescue story.

## Build and run

On Windows 11, see [Windows build and Direct3D 12 setup](docs/WINDOWS.md).

On Linux, run `./run.sh` after building from source. A locally built executable takes precedence over a bundled executable from a previous release.

Building from source requires a C++20 compiler, CMake, and the SDL2, libwebp, libpng16, libsndfile and json-c runtime libraries plus an OpenGL 3.3 core driver. Development packages are not needed because the platform ABI is declared in `src/platform.hpp`. On Debian or Ubuntu, `./install_deps.sh` installs everything.

```bash
./build.sh
./run.sh
```

`./build.sh` always starts from an empty `build/` directory. It prints the host, toolchain, configuration, every compile step, warnings, artifact checks (size, linked libraries, asset counts) and the unit test results, with a per-phase timing summary. The full log is saved to `build.log`. Options: `--verbose` shows every compiler command, `--no-tests` skips the unit tests and `--debug` builds without optimization. Other arguments go to CMake, for example `-DVOIDLINE_LTO=OFF`.

The executable is `build/voidline/voidline`. The build creates `build/voidline/assets/game.vpak` with all artwork, fonts and sound recordings, and loads them directly from memory. A Windows build requires Python 3 to build the same media pack. No external media extraction utility is needed at runtime. Media bundling deters casual browsing, but is not cryptographic DRM: a determined user can recover packaged assets. The game starts in fullscreen. Use `./run.sh --windowed` or F11 to switch window modes. Settings includes High and Performance rendering modes.

A successful build also stages a copyable runtime folder. On Linux it is `build/dist/VOIDLINE`. On Windows it is `build-windows/dist/VOIDLINE` after a Release build. Ship **only that folder** as a unit: it contains a single opaque asset archive, not the original image/sound files. Its build-size report lists the executable, assets, libraries and total payload. Testers can launch from that folder with `run.sh` or `run.bat`.

Progress lives in `${XDG_DATA_HOME:-$HOME/.local/share}/voidline/campaign.json`. Use `--save-file /path/to/campaign.json` for a custom location. Campaigns, settings, best sector records and armory purchases persist. Field checkpoints are available for retries during the current mission.

## Two-player co-op

1. Both players launch VOIDLINE **2.9.8** and open **Multiplayer**.
2. On the host, choose **Host Game**, select a difficulty, and select **Generate Session Key**. The lobby waits for player 2. Copy the key and share it with your partner.
3. On the other computer, choose **Join Game**, paste the key, and select **Connect**. After host approval, **each player selects and locks in a character** from the six-operative roster. The mission does not start until both lock in. The shared prologue plays before deployment and either player can request to skip it for both. They deploy together in **Chapter 1, The Descent**.

Use a local network or an IPv4 VPN. The host menu fills in a local IPv4 address. Edit it before generating the key if your partner reaches you through a different interface. The key contains that address and a random session identifier. The connection goes directly to the host on TCP port 38274. A key alone cannot connect through an unreachable address, and the game has no relay or matchmaking server.

Each player controls a separate operative and camera. The host simulates enemies, projectiles, pickups, checkpoints and explosive crates for both players at 120 fixed steps per simulation second. It sends up to 60 world snapshots per real second. The guest interpolates motion for operatives, enemies, lifts and projectiles. When the connection falls behind, the host sends its latest available state once the send queue clears. Mission transitions and retries use the same loading and deployment handshake. Bullet time slows the shared mission. Clear the guardian and bring both operatives to the extraction gate. Either operative going down ends the attempt, and the host can retry from the team's latest checkpoint. The host can advance the same session through the remaining sectors.

The co-op menu leaves the mission running for your partner. Closing the session or disconnecting returns the other player to Multiplayer. Co-op progress lasts for the session and unlocks equipment as the team clears sectors. Solo save slots remain independent.

## Enemy escalation and new operatives (v2.9.5)

Five enemy roles join the original five, with deterministic introductions and increasing late-game frequency. **Scavengers** (sector 1 onward) are weak fast melee flankers; **marksmen** (sector 4 onward) telegraph accurate long-range fire; **bulwarks** (sector 7 onward) use a front-facing shield that reduces incoming damage; **interceptors** (sector 10 onward) fly in fast evasive patterns and fire twin shots; **juggernauts** (sector 15 onward) are heavily armored infantry with seven-projectile suppression volleys. Higher sectors replace an increasing proportion of the appropriate ground or airborne enemy placements while retaining guardian encounters, checkpoint positions, route lengths and total placements.

The six selectable operatives are Mara Voss, Iona Rook, Kade Vale, **Jax Mercer** (male, reinforced breacher suit), **Darius Cole** (male, fast helmeted recon ranger) and **Sera Nyx** (female, ammunition specialist). All six have distinct armor palettes, character portraits, suit integrity/movement values and starting ammo capacity where applicable. New portraits are original modifications of the existing project portrait art and are included in this update. New campaigns use a 2-by-3 operative selector; existing saves and the three save slots remain valid. Multiplayer world snapshots accept the new operative and enemy IDs (use matching v2.9.6 builds for co-op).

On startup, VOIDLINE now plays a **3.2-second animated signal-acquisition splash** with radar rings, moving grid, scan light and startup progress. Any key, click or controller press skips directly to the menu without triggering a menu action. This splash animation does **not** change the intentionally static story cutscenes in v2.9.4.

## Two-page cutscenes and Windows/Linux co-op (v2.9.6)

After clearing a sector, the **first page** now shows one static, painted environment picture with the complete three-line chapter story. No live character models, camera animation, or animated cutscene overlays are rendered. Select **Level Summary** (or press Esc) to move to **page two**, which gives mission score, targets defeated, completion time, and sector progress. The story page also advances after its reading interval. In co-op the host advances both players together. The separate animated *startup splash* is unchanged; story sequences remain static.

Linux and Windows use the same C++ simulation, enemy and operative types, 120 Hz host-side update logic, binary world-state format, packet framing, protocol version, and shared gameplay assets. The Windows build uses Visual Studio/vcpkg as described in `docs/WINDOWS.md`. **Version 2.9.6 clients only** can play together; older VL4 session keys are not compatible with the updated approval handshake. This release was built/tested on Linux; a real Windows build and Windows-to-Linux playtest are still required before treating cross-platform operation as verified.

**Host a LAN mission:** Select Multiplayer → Host Game, choose the IPv4 address, and generate a session key. The host listens on TCP port **38274** (default) and advertises discovery on UDP **38275**. Other players can still connect by typing the private session key, even when LAN broadcast is unavailable. **Join a LAN mission:** Select Multiplayer → Join Game → **Find Local Hosts**. A list of discoverable games appears with the computer name and IP address; choose **Request Join**. Every incoming request, whether from LAN discovery or the session key, pauses at a host prompt showing the requester's IP address. The host must explicitly **Accept** or **Decline**. Rejected guests see a clear refusal message, and the host can accept another candidate. Join requests expire after 30 seconds without approval. The host stops advertising as soon as its two-player session fills.

LAN discovery broadcasts only a version tag and a public address/port; it never publishes the session key or sends gameplay state over UDP. Gameplay uses the existing authoritative TCP stream. LAN broadcast is usually restricted to one network segment; VPNs, guest Wi-Fi isolation, router/firewall policies, and multiple network interfaces may block it. Use the session key as fallback. These connections are not end-to-end encrypted; host only on networks you trust.

## Expanded campaign (v2.9.3)

All twenty sectors now grow progressively longer, from 4,030 to 19,768 world units. The original five hand-authored missions gain one to five additional combat/traversal sections **before** their existing guardian arenas. Later missions are assembled from biome-specific sections in deliberately staggered orders, with more lifts, broken paths, crossfire, vents, explosives, patrols and elite units. Enemy placements progress from 11 in Sector 1 to 92 in Sector 20, while supplies and checkpoints scale with the length. Stage lengths and encounter counts increase strictly with each new sector. The campaign map displays route length in world units. All of this works with existing campaign saves, including co-op and checkpoints.

## Chapter cutscenes

Version 2.9.4 originally presented the opening and between-level story as **static illustrated panels**. Actors, camera, effects and transitions do not animate within a panel. Text is displayed immediately, and discrete page markers replace moving progress bars. Panels automatically advance to retain all dialogue, and Skip still works. The Start Campaign / Skip Scene mouse click is now suppressed until released when gameplay starts, preventing an unintended first shot, including in multiplayer.

Version 2.9.3 extends the five-shot sequence to six cinematic shots, introducing a wide establishing view of the sector objective before the operative enters. Scene transitions hold the outgoing pose, while a slower camera dolly and synchronized acceleration / braking keep footsteps grounded. Close-up gestures and the objective reveal have been retimed so the acting and environmental action breathe. The final sequence lasts 33.2 seconds (36 seconds for the finale) and remains skippable. Version 2.9.1 refined character animation with acceleration/braking curves that preserve foot-to-ground movement, subtler breathing and weight transfer, and additional close-up suit plating, facial shading and equipment detail. Scene objects now draw behind the operatives, and subtitle mattes occupy the lower safe area instead of darkening the characters. Shot dissolves hold the outgoing frame's final pose.

Until v2.9.5 every successful extraction played a six-shot scene with the operative composited over the background. In v2.9.6 this has been replaced by the two-page story/summary described above. The story runs from the first distress call to Vega-9's evacuation. The restored story artwork conveys the rescue storyline through static environments and chapter dialogue. No characters are drawn in the story panel.

Select **Level Summary** or press **Esc** to continue. In co-op the host controls skipping for both players. Matching Linux and Windows builds use VL5 keys, or LAN discovery with host approval. Solo progress is saved when the sector clears, before playback starts.

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

**Weapon visuals (v2.9.2):** all seven weapons share side-view WebP artwork, muzzle anchoring, hand contact points and armory presentation. Pistol, SMG, launcher and railgun use new detailed sprites in `assets/`, built reproducibly with `scripts/generate-weapon-art.py` (optional developer tool). Runtime does not require Python.

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

Render a short, deterministic scene through the renderer with an offscreen SDL driver. Scenes include `splash`, `enemy-roster`, `operative0` to `operative5`, and `cinematic-establish`, `cinematic-arrival`, `cinematic-close`, `cinematic-detail`, `cinematic-reaction`, `cinematic-aftermath`, `cinematic-recover`, `1` to `20`, `map`, `menu`, `create`, `armory`, `intro`, `stand0` to `stand6`, `flip` and `slow`, `walk`, `arms`, `armsrun`, `boss`, `controls`, `hit0` to `hit4` (one enemy type shot to death) and `gallery0` to `gallery2`. Add `--follow` to crop the screenshot around the action.

```bash
SDL_VIDEODRIVER=offscreen SDL_AUDIODRIVER=dummy \
  ./run.sh --smoke-test --windowed --scene slow --frames 90 --screenshot slow.ppm
```

## Frame counter, synchronized co-op pause and momentum combat (v2.9.8)

**FPS:** Settings → Show FPS counter toggles a small measured FPS readout. Settings → FPS display corner cycles top left / top right / bottom left / bottom right. The settings persist in the existing campaign JSON; the default is off. The readout uses real frame time, including graphics presentation, averaged over about half a second. It is positioned clear of the main gameplay HUD.

**Co-op pause:** Esc, P or controller Pause freezes the simulation for **both players**, including enemies, timers, bullets and objectives. The initiating player is the only one who can select **RESUME GAME**. The partner sees the message **Waiting for other player** and cannot resume for them. A guest's pause/resume request goes through the host, which announces the authoritative state to both clients. Incoming TCP packets continue to be processed so disconnects and pause/resume commands still work. Pausing never starts a new mission or changes save data.

**Momentum:** Three consecutive kills without losing the streak now grant an extra focus boost and score bonus; every fifth kill in a streak partially refills the current magazine. Kill-chain callouts and ring/audio cues make the rewards visible. Being wounded or allowing the combo timer to expire resets the chain. This adds no new saved data.

The multiplayer protocol is **VL7**, which requires matching v2.9.8 clients on Linux and Windows; older VL6 clients cannot join. Windows compilation and mixed-OS connection must still be verified on native Windows hardware.

## Animated prologue and bundled assets (v2.9.7)

A new seven-shot 31.5-second prologue explains the Europa colony, the Tideworks flood, the Helix machine lockdown, Neri's distress call, the Aster rescue cutter, and the mission to restore the Terminus uplink. Every shot has continuous camera moves and procedural animated signal, radar, atmospheric dust and rescue-ship effects. Solo and co-op prologues can be skipped by mouse, Enter, Space or Escape; in co-op either player can skip and the host advances the scheduled deployment for both. The animated startup splash is unchanged; **post-level story stills and the following level-summary page remain static**. No operative model is superimposed on the prologue artwork.

The `scripts/pack-assets.py` tool builds `game.vpak` (all original artwork, audio, font atlases and font metrics) for both platforms. Runtime image/audio decoders now read the archive in memory, including OGG music through libsndfile virtual I/O. The copyable tester runtime contains **only `assets/game.vpak`**, the executable, libraries, launch scripts and required license notices. Source media remain in the developer tree to allow editing/repacking; do not distribute the source tree as the tester runtime. The package is obfuscated to stop direct opening as a picture/audio track, not encrypted DRM.

The multiplayer protocol is now `VL7`; old `VL6` (v2.9.7) game builds cannot connect. Windows and Linux use the same byte-level session packets, player choice handshake and world serialization. Native Windows compilation and an actual mixed-OS co-op session must still be checked on Windows hardware.

## Combat effects, expanded sectors and visual-overhaul pass (v2.9.9)

This update strengthens combat readability and spectacle with heavier enemy hit sparks, stronger player damage feedback and blood-like red hit spray.

Every level now starts with a safer insertion zone, longer traversal and additional vertical routes, lifts, ledges and cover obstacles. Mission generation was extended so later sectors play longer and feel denser instead of just wider.

Rendering received a broader atmosphere pass: denser backdrops, stronger bloom/glow, boosted light energy and deeper post-processing. Enemy silhouettes also gained more shared detail and movement patterns were refined for drones, interceptors, stalkers, scavengers and snipers.

## VOIDLINE v2.10.0 — Cooperative puzzle missions and visual combat pass

The multiplayer lobby now offers two distinct modes. **Campaign** retains the familiar 20 shared-route missions. **Cooperative** contains six new two-route levels where the host begins on an upper gantry and the guest begins on the lower floor. Each route has relays that open doors for the other player; the operator must hold the pad until the teammate passes the gate, after which the gate stays open. Both operatives must reach extraction. The game host remains authoritative over relay charge and completion.

Host setup is a mode/difficulty choice followed by sharing a key or accepting a discovered LAN guest. Join now automatically discovers local hosts and keeps manual session-key entry as an alternative. Both players select and lock their operatives before deployment. The server advertises the chosen game mode to the guest lobby after host approval.

Boss encounters escalate at two health thresholds, with extra volleys and distinct feedback. Each level has exploration-triggered sector-specific scripted set pieces. More red blood-like hit particles fall to the floor and briefly stain it, while robot hits retain metal sparks.

The network session key protocol is now VL8. Both players need v2.10.0; no loose artwork/audio is introduced, and `game.vpak` remains the runtime asset source.


## Cooperative shared insertion and split-route missions (v2.10.1)

In the six Cooperative missions, **both operatives spawn side by side on the same ground-level insertion deck**. Follow the signs at the fork: **Player 1 climbs the fixed steps or elevator to the upper route**, while **Player 2 continues through the lower industrial route**. Alternating objectives require one player to hold a relay pad while the other passes a locked gate. Each receiver must cross before the gate is permanently secured; the roles alternate on subsequent relays.

Checkpoints regroup both players safely at floor level, with nearby re-entry steps for the upper route. The multiplayer lobby now correctly delivers the game-mode announcement before character lock-in. Cooperative multiplayer uses the **VL9** session key and versioned LAN discovery/handshake, rejecting older 2.10.0 builds instead of mixing incompatible stage geometry. Both players must use v2.10.1.

## Mechanical co-op doors and varied sectors (v2.10.2)

Cooperative mode retains the shared starting bay. The separated upper and lower routes now use animated mechanical sliding doors, floor pressure plates and **E**-operated levers. One partner holds a pressure plate or engages a six-second lever while the other crosses. Once crossed, the gate is secured permanently, and a checkpoint will not bypass unsolved doors. Each player must alternate operating the controls and passing the doors.

The 20-sector campaign is longer, substantially harder, and uses varied biome- and section-specific traversals (stepped shafts, maintenance lifts, bridge paths, elevated firefights, flooded deck paths and high gantries). Cooperative missions are longer too, with more doors and changing encounter layouts. The multiplayer protocol is **VL10**; both computers must run 2.10.2.

Input: **E** operates levers. Weapon cycling remains available via the mouse wheel or **V**. Keyboard bindings can be customized in Settings.

## Independent graphics quality settings (v2.10.3)

Settings > Graphics complexity contains Performance, Balanced, Quality, Ultra and Custom presets,
plus individual rendering resolution, dynamic scaling, lighting, shadow quality, material relief, FXAA,
color grading, bloom, glow intensity, atmospheric fog, reflections, water shader detail, light shafts,
particles and background/scenery complexity. Changes are saved and used by the next rendered frame.
All options modify the local renderer only; multiplayer protocol and gameplay state are unchanged.

## Visible crosshair, safe enemy footing and shouldered weapons (v2.10.4)

The aiming reticle now uses high-contrast luminous lime with a dark five-pixel outline for visibility over any biome and muzzle flash. Grounded enemies must keep their leading foot supported by real ground or a ledge, and cannot walk across gaps or skip a pit at high speeds. Flying enemies retain their aerial behavior. Operatives and armed enemies now use smoother two-handed weapon shoulder presentation; raising the rifle transitions from low carry while firing immediately brings it up. The rendered rifle muzzle remains aligned with projectile origin once shouldered.
