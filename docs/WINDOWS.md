# Windows 11 compatibility

For cross-platform LAN sessions, use matching clients with the VL5 protocol and approval handshake in VOIDLINE 2.9.6. Older VL4 keys are rejected.

## Build

Use Visual Studio 2022 or its Build Tools with **Desktop development with C++**, a Windows SDK from 10.0.22621 onward, CMake 3.21 or newer, Git, and a bootstrapped [vcpkg checkout](https://learn.microsoft.com/en-us/vcpkg/get_started/get-started). Run the following from the patched game folder in a Visual Studio Developer Command Prompt. Substitute your vcpkg directory.

```bat
set "VCPKG_ROOT=C:\dev\vcpkg"
build.bat
```

The first build installs the manifest dependencies through vcpkg. It needs internet access for any packages missing from your vcpkg cache. The build runs the test suite and stages the executable, app-local libraries and assets in `build-windows\dist\VOIDLINE`. Launch its `run.bat` and see `BUILD-SIZES.txt` for the breakdown. If using a newer Visual Studio version, pass its CMake generator to `build.bat -Generator`. Debug builds stage in `build-windows\dist\VOIDLINE-Debug`.

## Install the Direct3D 12 compatibility runtime

Download and extract the **MSVC release** archive from [Mesa for Windows](https://github.com/pal1000/mesa-dist-win/releases/tag/25.3.4). The extracted release must contain x64 `opengl32.dll` and `dxil.dll`, plus `libgallium_wgl.dll` when the driver is split across DLLs. Pass either the extracted release directory or its `x64` directory.

```bat
install-mesa.bat "C:\Downloads\mesa3d-25.3.4-release-msvc"
run.bat --renderer dx12
```

The installer copies the Mesa runtime into the game's build directory and refreshes the staged tester folder. The game loads it for the Direct3D 12 modes.

| Launch option | Behavior |
| --- | --- |
| `--renderer auto` | Default on Windows. Tries hardware Direct3D 12, then the system OpenGL driver, then Direct3D 12 WARP. |
| `--renderer dx12` | Requires hardware Direct3D 12 via Mesa. Reports an error if the selected renderer does not match. |
| `--renderer warp` | Requires Direct3D 12 WARP via Mesa. Reduces internal render resolution for CPU rendering. |
| `--renderer opengl` | Uses the installed Windows OpenGL driver. |
| `--adapter "Radeon"` | Selects a Direct3D 12 adapter by a case-insensitive substring of its name. |
| `--mesa-path PATH` | Loads the Mesa DLLs from another directory. |
| `--dx-report` | Lists adapters, feature levels, shader models, resource binding, target blending, DXR, mesh shader, VRS, sampler feedback, enhanced barriers and DXGI tearing support. |

VOIDLINE submits its existing graphics pipeline through [Mesa's Direct3D 12 driver](https://docs.mesa3d.org/drivers/d3d12.html). The lighting, bloom, reflections, fog, particles, tone mapping and antialiasing use the same shaders as the Linux game. The engine probes its floating-point render target format at startup. DXR, mesh shaders, VRS and sampler feedback are capability diagnostics. The game does not submit those Direct3D workloads, and the floating-point render targets are used for effects in an SDR output pipeline.

## Saves and multiplayer

Windows stores campaigns and settings in `%LOCALAPPDATA%\VOIDLINE\campaign.json`. To load an existing Linux save, copy its JSON file there or point `run.bat --save-file` at the copied file. To host on Windows, the other player must be able to reach your selected IPv4 address over **TCP 38274** (unless using a custom port). Open **UDP 38275 inbound and outbound** in Windows Defender Firewall for the *Find Local Hosts* feature. The game uses one shared C++ TCP protocol on both Windows and Linux, so matching 2.9.8 builds use the same message format and world-state simulation. Host approval is required for LAN requests and for key-based requests. A discovered host stops advertising after accepting player two.

Typical flow: the host selects Multiplayer → Host Game → Generate Session Key. The guest selects Multiplayer → Join Game → Find Local Hosts → Request Join, or enters a VL5 session key. The host must choose Accept to let the guest deploy. Decline and timeout return the host to waiting. For a VPN or routed network that blocks broadcast, use the session key; there is no internet lobby server.

LAN discovery and the TCP co-op protocol are not encrypted; use trusted networks. The program does not send the private session key in UDP discovery advertisements. A `VL7-` key embeds the host IP, port, and a random identifier, and should only be shared with intended players.

**Cross-platform verification:** run the same 2.9.6 source revision on both operating systems; run `verify-windows.bat` and `ctest --test-dir build-windows -C Release --output-on-failure` on Windows. Host on Windows, join from Linux using Find Local Hosts, accept the request and play through a checkpoint and extraction. Repeat with Linux hosting and Windows joining, then repeat with the session key. This needs actual Windows hardware or a VM with networking; tests in this Linux development environment do not substitute for that run.

## Verification

```bat
verify-windows.bat
run.bat --renderer warp --windowed --smoke-test --scene 20 --frames 3
```

The first command reports the Direct3D device capabilities and renders a short hardware smoke test. The second checks the WARP path separately. The graphics mode and driver appear in the startup output. The 2.9.0 tester package was cross-compiled on Linux and checked for loader dependencies. Native Windows 11 rendering still needs a smoke test on the target hardware.

### 2.9.7 packaged media and LAN character selection

Install **Python 3** (available in PATH for CMake's `find_package(Python3)`) before building. CMake creates `build-windows/voidline/assets/game.vpak` instead of copying separate images and audio. Release staging copies only this archive and required license notices. Both players running protocol VL7 select their operatives independently and lock in after host approval. For LAN discovery, allow UDP 38275 and TCP 38274 on the host. Cross-OS multiplayer still requires final Windows hardware verification.

### 2.9.8 FPS and synchronized co-op pause

The Settings menu can display an FPS counter in any display corner. FPS is calculated from the actual swap/presentation cadence, not the fixed simulation step. Both Windows and Linux save these options in campaign.json.

Multiplayer now uses `VL7` session keys. Esc/P/controller Pause on either computer freezes the authoritative host simulation and both local interfaces; only the initiating player can resume. Host-directed state packets include the mission generation and the initiating peer, and the guest cannot override an existing pause initiated by the host. To connect across platforms both installations must run v2.9.8 or later with the same protocol. TCP 38274 and UDP 38275 firewall requirements are unchanged.


### 2.9.9 combat, level and rendering expansion

This update keeps the `VL7` protocol but extends sector generation, enemy feedback and visual effects. Both Windows and Linux players can stay cross-compatible as long as both sides run v2.9.9 or another `VL7` build. The source update adds a safer start area per sector, longer mission layouts, more vertical traversal objects, richer backdrops and stronger bloom/glow/shadow presentation.


### v2.10.0 cooperative puzzles

Both platforms build the same C++ simulation and byte-order-stable VL8 packets. Multiplayer now offers Campaign or Cooperative modes. The six Cooperative missions include player-specific upper and lower routes, alternating team relay gates and shared completion. Host approval is unchanged. LAN discovery starts when opening Join. The mode is announced after host approval; guests do not need the session key when using LAN. Rebuild Windows with `build.bat` after applying the update. Cross-platform operation still requires a real Windows/Linux hardware test.


### 2.10.1 cooperative insertion layout

The Cooperative missions now deploy both players at the same safe ground-level insertion area. At the signposted fork, the host climbs to the upper switch route and the joining player advances on the lower route; their relay/door duties alternate. The new geometry, mission hints and checkpoint re-entry steps are shared C++ data on Linux and Windows. The session key is now **VL9**; versioned LAN discovery and the TCP join handshake reject older peers to prevent mismatched level collisions. **Both peers must install v2.10.1**.

The v2.10.1 patch also fixes acceptance of the lobby-mode packet on the shared network receive path so the joining client can see the selected Cooperative mode.

### 2.10.2 cooperative doors and expanded campaign

Both Linux and Windows use the same C++ pressure plate, lever, synchronized door shutter and extended level generators. Hold **E** when near a lever (or rebind the Interact action); plates trigger by standing on them. The mouse wheel or **V** selects the next weapon. Session keys now start with **VL10-** and LAN advertisements use VLD8. Both peers must run the same version because world packets include door opening and lever-timer state. Build with `build.bat` after applying the update. Windows native multiplayer remains to be tested on Windows hardware.
