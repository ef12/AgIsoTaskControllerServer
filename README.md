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
- CAN drivers: virtual CAN bus (works everywhere, no hardware) and SocketCAN (Linux)

## Protocol notes

- A client's DDOP tree is shown once the client **stores its pool to NVM**
  (the transfer itself is not retained by the server by design). You can also
  load any DDOP binary (`.bin`/`.iop`) manually per client for visualization.
- TODO: match structure/localization labels against stored pools; forward the
  CAN stack logger into the GUI log; persistent settings.

## Build

Requires CMake 3.21+, a C++17 compiler, and Qt 6.5+ (`Core`, `Quick`, `Quick3D`, `Qml`).
CI builds Windows / Linux / macOS automatically.

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

## Usage

1. Pick a driver: `virtual` + a bus name (e.g. `TC-Server`) works with no hardware —
   run an implement simulator on the same virtual bus name to see clients appear.
   On Linux with real hardware, use `socketcan` + interface (e.g. `can0`).
2. Set TC number, booms, sections, channels; press **Start server**.
3. Select a client, inspect its DDOP, watch live values, send commands.

## License

GPL-3.0. See [LICENSE](LICENSE).
