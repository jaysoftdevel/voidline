# Windows 11 compatibility

This update adds a Windows x64 build. Multiplayer keeps the VOIDLINE 2.4.0 wire format for cross-platform LAN sessions.

## Build

Use Visual Studio 2022 or its Build Tools with **Desktop development with C++**, a Windows SDK from 10.0.22621 onward, CMake 3.21 or newer, Git, and a bootstrapped [vcpkg checkout](https://learn.microsoft.com/en-us/vcpkg/get_started/get-started). Run the following from the patched game folder in a Visual Studio Developer Command Prompt. Substitute your vcpkg directory.

```bat
set "VCPKG_ROOT=C:\dev\vcpkg"
build.bat
```

The first build installs the manifest dependencies through vcpkg. It needs internet access for any packages missing from your vcpkg cache. The build runs the test suite and places `voidline.exe`, its dependencies, and the game assets in `build-windows\voidline\Release`. If using a newer Visual Studio version, pass its CMake generator to `build.bat -Generator`. For Debug, use `build.bat -Configuration Debug` and run the executable in the Debug folder.

## Install the Direct3D 12 compatibility runtime

Download and extract the **MSVC release** archive from [Mesa for Windows](https://github.com/pal1000/mesa-dist-win/releases/tag/25.3.4). The extracted release must contain x64 `opengl32.dll` and `dxil.dll`, plus `libgallium_wgl.dll` when the driver is split across DLLs. Pass either the extracted release directory or its `x64` directory.

```bat
install-mesa.bat "C:\Downloads\mesa3d-25.3.4-release-msvc"
run.bat --renderer dx12
```

The installer copies the Mesa runtime into the game's build directory. The game loads it only for the Direct3D 12 modes.

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

Windows stores campaigns and settings in `%LOCALAPPDATA%\VOIDLINE\campaign.json`. To load an existing Linux save, copy its JSON file there or point `run.bat --save-file` at the copied file. To host on Windows, the other player must be able to reach your selected IPv4 address over TCP port 38274.

## Verification

```bat
verify-windows.bat
run.bat --renderer warp --windowed --smoke-test --scene 20 --frames 3
```

The first command reports the Direct3D device capabilities and renders a short hardware smoke test. The second checks the WARP path separately. The graphics mode and driver appear in the startup output. Linux regression tests and the update installer were checked during preparation. The Windows executable and graphics runtime were not built or run in the Linux packaging environment.
