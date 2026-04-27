# High FPS Physics Fix

An F4SE plugin for **Fallout 4** that decouples physics, animation, and timing
from framerate, plus DXGI/swap-chain replacements for windowing, VSync and a
configurable framerate limiter.

This is a maintained fork of the original
[AntoniX35/HighFPSPhysicsFix](https://www.nexusmods.com/fallout4/mods/44798/)
([upstream repo](https://github.com/AntoniX35/HighFPSPhysicsFix)). It ships a
**single DLL** that loads on every supported runtime via Address Library and
[Dear-Modding-FO4/CommonLibF4](https://github.com/Dear-Modding-FO4/commonlibf4).

## Supported runtimes

One build, multiple games:

| Runtime | Game version            | Notes                |
|---------|-------------------------|----------------------|
| OG      | 1.10.163                | F4SE 0.6.23          |
| NG      | 1.10.984+               | F4SE 0.7.x next-gen  |
| AE      | 1.11.x                  | F4SE 0.7.x AE        |

Address Library is required (V0 format, F4SE).

## Features

- Untie game speed, physics, animation, wind, rotation, lockpicking and other
  framerate-tied subsystems from the actual frame rate.
- Configurable framerate limiter with separate caps for game / interior /
  exterior / loading / lockpicking / Pip-Boy / out-of-focus.
- DXGI swap-chain control: VSync toggle, present interval, swap effect,
  buffer count, tearing, scaling mode, borderless upscaling.
- Window management: cursor lock, force-minimize on focus loss, ghosting
  off, automatic centering, position offsets.
- Optional on-screen display (FPS / frametime / frame counter / VRAM).
- Papyrus dynamic update budget that tracks the realised frame time.
- Several long-standing engine fixes (white-screen, stuck animations,
  workshop rotation, loading-model speed, motion-responsiveness).

Every feature is opt-in / opt-out in `HighFPSPhysicsFix.ini`.

## Install (users)

1. Install [F4SE](https://f4se.silverlock.org/) for your runtime.
2. Install [Address Library for F4SE](https://www.nexusmods.com/fallout4/mods/47327).
3. Drop `HighFPSPhysicsFix.dll` and `HighFPSPhysicsFix.ini` into
   `Data\F4SE\Plugins\`.
4. Edit `HighFPSPhysicsFix.ini` to taste.

The log lives at
`Documents\My Games\Fallout4\F4SE\HighFPSPhysicsFix.log` (rotating, 5 MiB x 3).

## Build (developers)

Requires Visual Studio 2022 (v143 toolset), CMake 3.21+ and
[vcpkg](https://github.com/microsoft/vcpkg).

```powershell
git clone --recurse-submodules https://github.com/AntoniX35/High-FPS-Physics-Fix.git
cd High-FPS-Physics-Fix

$env:VCPKG_ROOT = 'C:\path\to\vcpkg'
# Optional: enables the post-build copy step.
$env:Fallout4Path = 'C:\Games\Fallout 4'

cmake --preset vs2022-windows-vcpkg
cmake --build build --config Release
```

Output: `build\Release\HighFPSPhysicsFix.dll`.

CommonLibF4 is vendored as a submodule at `extern/CommonLibF4` (Dear-Modding-FO4
fork at the last commit before the switch to xmake).

## Configuration

All keys live in `HighFPSPhysicsFix.ini` (deployed alongside the DLL). The
companion file `HighFPSPhysicsFix_Custom.ini`, if present, overrides the main
file key by key.

### `[Main]`

| Key | Default | Description |
|---|---|---|
| `UntieSpeedFromFPS` | `true` | Untie game speed from framerate. |
| `DisableiFPSClamp` | `true` | Disable the `iFPSClamp` game option, which would otherwise interfere. |
| `EnableVSync` | `true` | Enable vertical sync. ENB `ForceVSync` overrides this. |
| `VSyncPresentInterval` | `1` | Present interval (1-4). Same as `iPresentInterval`. |
| `AllowTearing` | `true` | Allow tearing in borderless / windowed (required for VSync off). |
| `DisableVSyncWhileLoading` | `true` | Disable VSync on loading screens. |
| `DisableVSyncWhileLockpicking` | `false` | Disable VSync while lockpicking. |
| `DisableBlackLoadingScreens` | `false` | Skip black loading screens. |
| `DisableAnimationOnLoadingScreens` | `false` | Skip loading-screen animation (much faster loads). |
| `PostloadingMenuSpeed` | `1.0` | Post-loading animation speed multiplier. |

### `[Limiter]`

| Key | Default | Description |
|---|---|---|
| `InGameFPS` | `0.0` | Global FPS cap (0 = off). |
| `ExteriorFPS` | `0.0` | Cap in exteriors (0 = off). |
| `InteriorFPS` | `0.0` | Cap in interiors (0 = off). |
| `LoadingScreenFPS` | `350.0` | Cap during loading screens. Requires `DisableVSyncWhileLoading=true`. |
| `LockpickingFPS` | `60.0` | Cap during lockpicking. |
| `PipBoyFPS` | `0.0` | Cap inside the Pip-Boy (helps FallUI). |
| `OutOfFocusFPS` | `60.0` | Cap when the game window is unfocused. |
| `OneThreadWhileLoading` | `true` | Use one CPU thread while loading a new game (works around quest bugs). |
| `FramerateLimitMode` | `0` | `0` = limit before present (smooth frametimes), `1` = after present (lower input lag). |

### `[Display]`

| Key | Default | Description |
|---|---|---|
| `Fullscreen` | (game) | Exclusive fullscreen. Commented out by default - inherits Fallout4Prefs.ini. |
| `Borderless` | (game) | Borderless windowed. Commented out by default. |
| `BorderlessUpscale` | `false` | Stretch to monitor in borderless fullscreen. |
| `Resolution` | (game) | `WIDTHxHEIGHT`. Commented out by default. |
| `ResolutionScale` | (game) | Scale factor instead of fixed resolution. |
| `ResizeBuffersDisable` | `false` | Don't resize swap-chain buffers on window resize. |
| `BufferCount` | `0` | Swap-chain buffer count (0 = auto, valid 1-8). |
| `SwapEffect` | `0` | `0` auto, `1` discard, `2` sequential, `3` flip_sequential, `4` flip_discard. |
| `ScalingMode` | `1` | `1` unspecified, `2` centered, `3` stretched. |

### `[Fixes]`

| Key | Default |
|---|---|
| `FixCPUThreads` | `true` |
| `FixStuttering` | `true` |
| `FixWhiteScreen` | `true` |
| `FixWindSpeed` | `true` |
| `FixRotationSpeed` | `true` |
| `FixSittingRotationSpeed` | `true` |
| `FixWorkshopRotationSpeed` | `true` |
| `FixLoadingModel` | `true` |
| `FixStuckAnimation` | `true` |
| `FixMotionResponsive` | `true` |

### `[Miscellaneous]`

| Key | Default | Description |
|---|---|---|
| `DisableActorFade` | `false` | Don't fade actors when the camera intersects. |
| `DisablePlayerFade` | `false` | Don't fade the player when the camera intersects. |

### `[Window]`

| Key | Default | Description |
|---|---|---|
| `LockCursor` | `true` | Confine the mouse to the game window. |
| `ForceMinimize` | `false` | Force-minimize on focus loss. |
| `DisableProcessWindowsGhosting` | `true` | Disable Windows ghosting (prevents "Not responding"). |
| `AutoCenter` | `false` | Center the window on its monitor at startup. |
| `OffsetX`, `OffsetY` | `0` | Window position offset relative to the primary monitor. |

### `[Papyrus]`

| Key | Default | Description |
|---|---|---|
| `DynamicUpdateBudget` | `false` | Adjust `fUpdateBudgetMS` based on previous cycle time. |
| `UpdateBudgetBase` | `1.2` | Budget at <=60 FPS (ms). Match `fUpdateBudgetMS` if you set one. |
| `BudgetMaxFPS` | `240` | Cap above which the budget no longer shrinks (60-300). |
| `OSDStatsEnabled` | `false` | Show `fUpdateBudgetMS` in the OSD. |

### `[OSD]`

| Key | Default | Description |
|---|---|---|
| `EnableOSD` | `false` | Master toggle for the on-screen display. |
| `FPS`, `Bare_FPS`, `FrameTime`, `Bare_FrameTime`, `Counter`, `VRAM`, `All` | see ini | Stats to display. |
| `LoadingTime`, `Bare_LoadingTime` | `false` | Show last loading-screen time. |
| `LoadingTimeDelay` | `5.0` | Seconds before the loading-time line disappears. |
| `UpdateInterval` | `0.3` | OSD refresh period (seconds). |
| `Align` | `1` | `1` TL, `2` TR, `3` BL, `4` BR. |
| `Offset` | `4 4` | OSD `X Y` offset (px). |
| `Scale` | `1.0 0.9` | Font `X Y` scale. |
| `AutoScale` | `true` | Auto-scale font with line count. |
| `ScaleToWindow` | `true` | Scale font to window size. |
| `FontFile` | `` | Custom MakeSpriteFont bitmap (place in `Data\F4SE\Plugins\HFonts`). |
| `Color` | `255 255 255 255` | RGBA. |
| `OutlineColor` | `0 0 0 255` | RGBA. |
| `OutlineOffset` | `1` | Outline offset in px. |

### `[Debug]`

| Key | Default | Description |
|---|---|---|
| `LogLevel` | `info` | One of `trace`, `debug`, `info`, `warn`, `error`, `critical`, `off`. |

## Credits

- **AntoniX35** - original author, [Nexus mod page](https://www.nexusmods.com/fallout4/mods/44798/),
  [upstream repository](https://github.com/AntoniX35/HighFPSPhysicsFix).
- **Dear-Modding-FO4** - [CommonLibF4](https://github.com/Dear-Modding-FO4/commonlibf4),
  the multi-runtime Address Library port that makes a single DLL possible.
- **ersh1** - additional swap-effect work merged upstream.

## License

MIT. See [`LICENSE.txt`](./LICENSE.txt) for the full text. The plugin is
copyright (c) 2025 AntoniX35.
