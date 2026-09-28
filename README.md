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
- TC-BAS: requests the client's default process data when its pool is active and
  at every task start
- TC-SC: switches the client to automatic section control while a task is active
  and turns each section on and off from the field boundary and the coverage,
  per boom (Setpoint Condensed Work State)
- Rate control: commands the client's settable rate setpoints while a task is active
- Coverage map: the ground each section applied, as the client reports it, in the
  3D view and the field map
- 3D section-control view: section boxes light up from the client's work states
- Event log console, TC identify banner
- CAN drivers: WCAN shared-memory bus (Windows, cross-process), PCAN-USB
  (Windows), PEAK PCAN Virtual via CAN-API 2 (Windows, cross-process),
  process-local virtual CAN (tests), and SocketCAN (Linux)

## Protocol notes

- **DDOP transfer.** A client may send its pool in several object pool transfers,
  each after a request of its own (for example the device object, then the
  process data, then the elements). They are joined into one pool, which is
  parsed once the client activates it. It is parsed with the DDOP layout of the
  TC version the client reported (version 4 adds the extended structure label).
  You can also load any DDOP binary (`.bin`/`.iop`) manually per client.
- **Stored pools** are kept in memory for as long as the server runs. A client
  that asks for the structure label of the stored pool gets it and can activate
  without uploading again; a client that sends its own label is told "stored"
  only when the labels match.
- **TC-BAS.** When a pool is activated, and again at every task start (a client
  may drop its measurements when the task stops), the server requests the
  client's default process data (DDI 0xDFFF) and asks for a report on every
  change of the work states and the section control state.
- **TC-SC.** While a task is active and **Automatic section control** (TC-SC tab)
  is on, the server sets the client's Section Control State to automatic and
  sends a Setpoint Condensed Work State per boom: on changes, and every second.
  A section is on when the machine moves, the point it reaches after the
  client's SC turn-on time (1 s if unknown) lies inside the selected field, and
  that ground was not covered before. Stopping the task, switching automatic
  section control off or stopping the server turns all sections off and sets
  the client back to manual. Set values are sent without acknowledge, which not
  every client accepts.
- **Rate control.** Settable rate setpoints in the DDOP (volume, mass or count per
  area, spacing, ...) are listed in the TC-SC tab. A target other than 0 is sent
  while a task is active, after setting the Prescription Control State to
  automatic where the client has one.
- **Coverage** follows the section states the client reports (Actual Condensed
  Work State), or the commanded ones for a client that reports none. Worked area
  counts covered ground once, however often it is driven over.
- **Geometry.** Element offsets are taken from the device reference point
  (ISO 11783-10), X forward and Y to the right; an element without an offset of
  its own sits where the element above it does. Offsets and widths may come as
  properties or as process data values, which the server requests.
- **Capacity.** The server logs when a client reports more booms, sections or
  channels than the TC offers, since a client then holds back what exceeds it.
- TODO: forward the CAN stack logger into the GUI log; persistent settings.

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
   PCAN-USB channel 1. Use `pcan_virtual` plus a CAN-API 2 network name (1..20
   bytes, default `PCANLight_USB`) to share a PEAK PCAN Virtual network with
   AgIsoVirtualTerminal's **PEAK PCAN Virtual** option and other CAN-API 2
   applications without CAN hardware. This requires the PEAK driver with its
   CAN-API 2 runtime (`CanApi2.dll` in the Windows system directory). A missing
   network is registered persistently at 250 kbit/s, so the applications can be
   started in any order. The `virtual` driver is useful only for participants in
   the same process. On Linux, use `socketcan` plus an interface such as `can0`.
2. Set TC number, booms, sections, channels; press **Start server**. Offer at
   least what the client reports (see the event log), e.g. 64 sections.
3. Select a client, inspect its DDOP, watch live values, send commands.
4. For section control: start GPS, create or select a field, create and start a
   task, and drive. The TC-SC tab shows the section states and the worked area.

### Command-line options

The options preset the top bar, so the server can be started from a script:

| Option | Meaning |
|---|---|
| `--driver NAME` | `wcan`, `pcan_usb`, `pcan_virtual`, `virtual` or `socketcan` |
| `--channel NAME` | Bus name, CAN-API 2 network name, or SocketCAN interface |
| `--tc-number N` | TC number, 1..32 |
| `--booms N`, `--sections N`, `--channels N` | What the TC reports it supports |
| `--autostart` | Start the server at once with these settings |

For example:
`AgIsoTaskControllerServer --driver pcan_virtual --channel PCANLight_USB --sections 64 --autostart`

## License

GPL-3.0. See [LICENSE](LICENSE).
