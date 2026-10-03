# AgIsoTaskControllerServer 🚜

**Ag**riculture **ISO**-11783 **Task Controller Server** — a free, open-source GUI terminal
that implements the **server side** of the ISOBUS Task Controller (ISO 11783-10).

Connect it to a CAN bus and it behaves like a tractor terminal's task controller:
implements send their Device Descriptor Object Pool (DDOP), stream process data,
and receive commands — all visualized and controllable from one window.

Built on [AgIsoStack++](https://github.com/ef12/AgIsoStack-plus-plus) (`TaskControllerServer`
+ `DeviceDescriptorObjectPool`) with a Qt 6 Quick / Qt Quick 3D interface.

## Features (v1 — monitor + control core)

- Interface: a dark and a light theme (switch at the bottom of the navigation rail),
  a header with the server and task state and their start/stop buttons, a navigation
  rail with four pages (Connect, Implements, GPS, Fields) in a resizable side panel,
  the 3D view, and the Task Controller data docked under it. The layout, the theme
  and the connection settings are remembered between sessions.
- Implements page: a card per TC client (source address, function, manufacturer,
  DDOP size, TC version, pool and timeout state, NAME), and the selected client's
  DDOP as an object inspector (DVC, DET, DPD, DPT and DVP objects in their tree), its
  declared DDIs with their live values, and the pool actions
- Connect page: the CAN interface, what the TC offers, and the devices heard on the
  bus, connected to this TC or not
- Live process-data table with per-(client, DDI, element) tracking
- Task start/stop (task-totals-active status bit)
- TC-BAS: requests the client's default process data when its pool is active and
  at every task start
- TC-SC: switches the client to automatic section control while a task is active
  and turns each section on and off from the field boundary and the coverage,
  per boom (Setpoint Condensed Work State)
- Rate control: commands the client's settable rate setpoints while a task is active
- Coverage map: the ground each section applied, as the client reports it, in the
  3D view and the field map
- 3D section-control view: each boom is an LED bar trailing the tractor, one LED per
  section, as wide as the section, lit while the section is on (from the client's work
  states). Booms that would lie on top of each other are drawn one behind the other.
  The booms ride on a trailer (tongue from the hitch, frame, transport and gauge
  wheels) behind a large orange cab tractor; all wheels turn with the distance driven
  and the front wheels steer. A ground grid marks every 10 m and 50 m. The vehicles
  are built from Qt Quick 3D primitives (`qml/TractorModel.qml`,
  `qml/ImplementTrailer.qml`, `qml/WheelModel.qml`), so no model files are needed.
  Over the view float the implement, the section bar, the camera tools (follow,
  fit field, zoom, reset, field map), the speed, heading, worked area and sections
  on, and for the simulated GPS a drive pad (steering wheel, set speed, stop).
- Section LED bars (the 3D view, the TC-SC tab and the map window): one bar per boom,
  drawn to scale and where the boom is across the implement, so a 31-row seeding boom
  shows 31 narrow LEDs and a 2-section fertilizer boom 2 wide ones
- Task Controller data: TC-Basic, TC-SC, raw process data, DDI traffic and the event
  log, docked under the 3D view (it folds down to its tabs) or in a window of its own.
  DDI traffic and the log follow the newest line; TC identify notice
- Field map window: the field, its boundary and the coverage in 2D, with tools to draw
  a field boundary
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
- TODO: forward the CAN stack logger into the GUI log.

## Build

Requires CMake 3.21+, a C++17 compiler, and Qt 6.5+ (`Core`, `Quick`,
`QuickControls2`, `Quick3D`, `QuickTimeline`, `ShaderTools`, and `Qml`). CI builds
Windows / Linux / macOS automatically.

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
git -C _dependencies/AgIsoStack-plus-plus checkout 9a320161189c2015a762c5f81dd346f4c167905f
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

1. On the **Connect** page, pick a driver. On Windows, `wcan` plus a shared bus
   name (for example `big_planter_isobus`) connects separate applications without
   CAN hardware; every application must use the same bus name. Use `pcan_usb` for PEAK
   PCAN-USB channel 1. Use `pcan_virtual` plus a CAN-API 2 network name (1..20
   bytes, default `PCANLight_USB`) to share a PEAK PCAN Virtual network with
   AgIsoVirtualTerminal's **PEAK PCAN Virtual** option and other CAN-API 2
   applications without CAN hardware. This requires the PEAK driver with its
   CAN-API 2 runtime (`CanApi2.dll` in the Windows system directory). A missing
   network is registered persistently at 250 kbit/s, so the applications can be
   started in any order. The `virtual` driver is useful only for participants in
   the same process. On Linux, use `socketcan` plus an interface such as `can0`.
2. Set TC number, booms, sections, channels; press **Start server**. Offer at
   least what the client reports (see the Event log tab), e.g. 64 sections.
3. On the **Implements** page, select a client, inspect its DDOP and watch live values.
4. For section control: start the GPS (**GPS** page), create or select a field and
   create and start a task (**Fields** page), and drive. With the simulated GPS, drive
   with the drive pad in the 3D view, or click the view and use W/S for the set speed,
   A/D to steer, C to centre the wheel and Space to stop (F follows the tractor). The
   TC-SC tab shows the section states and the worked area.

### Command-line options

The options preset the Connect page, over the settings remembered from the last
session, so the server can be started from a script:

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
