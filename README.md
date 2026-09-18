# AgIsoTaskControllerServer 🚜

**Ag**riculture **ISO**-11783 **Task Controller Server** — a free, open-source GUI terminal
that implements the **server side** of the ISOBUS Task Controller (ISO 11783-10).

Connect it to a CAN bus and it behaves like a tractor terminal's task controller:
implements send their Device Descriptor Object Pool (DDOP), stream process data,
and receive commands — all visualized and controllable from one window.

Built on [AgIsoStack++](https://github.com/ef12/AgIsoStack-plus-plus) (`TaskControllerServer`
+ `DeviceDescriptorObjectPool`) with a Qt 6 Quick / Qt Quick 3D interface.

## Features (v1 — monitor + control core)

- Client roster: source address, NAME, DDOP size, active/timeout state, TC version, status bits
- DDOP inspector: parsed device/element/process-data/property tree per client
- Live process-data table with per-(client, DDI, element) tracking
- Commands: request value, set value (± acknowledge), measurement triggers
  (time/distance interval, min/max/change thresholds)
- Task start/stop (task-totals-active status bit)
- 3D section-control view: section boxes light up from a configurable section-state DDI
- Event log console, TC identify banner
- CAN drivers: WCAN shared-memory bus (Windows, cross-process), PCAN-USB
  (Windows), process-local virtual CAN (tests), and SocketCAN (Linux)

## Protocol notes

- A client's DDOP tree is shown once the client **stores its pool to NVM**
  (the transfer itself is not retained by the server by design). You can also
  load any DDOP binary (`.bin`/`.iop`) manually per client for visualization.
- TODO: match structure/localization labels against stored pools; forward the
  CAN stack logger into the GUI log; persistent settings.

## Build

Requires CMake 3.21+, a C++17 compiler, and Qt 6.5+ (`Core`, `Quick`,
`Quick3D`, `QuickTimeline`, `ShaderTools`, and `Qml`). CI builds Windows /
Linux / macOS automatically.

```bash
git clone https://github.com/ef12/AgIsoTaskControllerServer.git
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
```

To build only the Qt-free core library (e.g. without Qt installed):

```bash
cmake -S . -B build -DAGISOTC_BUILD_GUI=OFF
cmake --build build --parallel
```

### Windows: WCAN driver library (one-time setup)

On Windows the stack always enables its WCAN shared-memory CAN driver, whose
prebuilt library is generated from the SIL repository. Do this once (PowerShell,
from the repo root):

```powershell
git clone https://github.com/ef12/AgIsoStack-plus-plus.git _dependencies/AgIsoStack-plus-plus
git -C _dependencies/AgIsoStack-plus-plus checkout 1eb0a89f21e0c2ea57a2c63b93a7d3b2218bc66f
git clone https://github.com/ef12/SIL.git _dependencies/SIL
git -C _dependencies/SIL checkout d8a869322c7a782bb197662deff67045dc353c7c
$vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio/Installer/vswhere.exe"
$vsInstall = (& $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath | Select-Object -First 1).Trim()
$env:WCAN_VCVARS = Join-Path $vsInstall "VC/Auxiliary/Build/vcvars64.bat"
./_dependencies/AgIsoStack-plus-plus/tools/sync_wcan.ps1 -Source ./_dependencies/SIL
```

Then always configure with the source-dir overrides (use a fresh `build` dir if a
previous configure failed half-way):

```powershell
Remove-Item -Recurse -Force build -ErrorAction SilentlyContinue
$stackSource = (Resolve-Path ./_dependencies/AgIsoStack-plus-plus).Path.Replace('\', '/')
$silSource = (Resolve-Path ./_dependencies/SIL).Path.Replace('\', '/')
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release `
  "-DFETCHCONTENT_SOURCE_DIR_CAN_STACK=$stackSource" `
  "-DFETCHCONTENT_SOURCE_DIR_SIL=$silSource"
cmake --build build --config Release --parallel
```

This needs Visual Studio 2022 with C++ tools, and Qt 6.5+ installed locally for
the GUI (`-DAGISOTC_BUILD_GUI=OFF` skips the Qt requirement).

## Usage

1. Pick a driver. On Windows, `wcan` plus a shared bus name (for example
   `big_planter_isobus`) connects separate applications without CAN hardware;
   every application must use the same bus name. Use `pcan_usb` for PEAK
   PCAN-USB channel 1. The `virtual` driver is useful only for participants in
   the same process. On Linux, use `socketcan` plus an interface such as `can0`.
2. Set TC number, booms, sections, channels; press **Start server**.
3. Select a client, inspect its DDOP, watch live values, send commands.

## License

GPL-3.0. See [LICENSE](LICENSE).
