import QtQuick
import QtCore

// What the server is started with: the CAN interface and what the TC reports it supports.
// Remembered between sessions; the command-line options (--driver, --channel, ...) override it
// at start-up, and --autostart starts the server once they are applied.
QtObject {
    id: config

    readonly property var drivers: [
        { key: "wcan", label: "WCAN shared memory", channelLabel: "Bus name", usesChannel: true,
          defaultChannel: "big_planter_isobus",
          hint: "Shared-memory bus between applications on this PC, no CAN hardware needed. Every application must use the same bus name." },
        { key: "pcan_usb", label: "PEAK PCAN-USB", channelLabel: "Channel", usesChannel: false,
          defaultChannel: "",
          hint: "PEAK PCAN-USB adapter, channel 1." },
        { key: "pcan_virtual", label: "PEAK PCAN Virtual", channelLabel: "Network name", usesChannel: true,
          defaultChannel: "PCANLight_USB",
          hint: "CAN-API 2 network shared with other PEAK applications such as AgIsoVirtualTerminal. Needs the PEAK driver; a missing network is registered at 250 kbit/s." },
        { key: "virtual", label: "In-process virtual", channelLabel: "Channel", usesChannel: true,
          defaultChannel: "",
          hint: "Only reaches participants in this process, so it is meant for tests." },
        { key: "socketcan", label: "SocketCAN (Linux)", channelLabel: "Interface", usesChannel: true,
          defaultChannel: "can0",
          hint: "Linux SocketCAN interface, for example can0." }
    ]

    property string driver: "wcan"
    property string channel: "big_planter_isobus"
    property int tcNumber: 1
    property int booms: 4
    property int sections: 64
    property int channels: 16

    readonly property int driverIndex: indexOfDriver(driver)
    readonly property var current: drivers[Math.max(0, driverIndex)]
    readonly property string summary: current.label + (current.usesChannel && channel !== "" ? " · " + channel : "")
                                      + " · TC " + tcNumber

    // The channel text per driver, so switching drivers keeps each bus or network name.
    property var channelPerDriver: ({})

    readonly property Settings settings: Settings {
        category: "server"
        property alias driver: config.driver
        property alias channel: config.channel
        property alias tcNumber: config.tcNumber
        property alias booms: config.booms
        property alias sections: config.sections
        property alias channels: config.channels
    }

    function indexOfDriver(key) {
        for (let i = 0; i < drivers.length; ++i) {
            if (drivers[i].key === key)
                return i
        }
        return -1
    }

    function selectDriver(key) {
        const index = indexOfDriver(key)
        if (index < 0 || key === driver)
            return
        channelPerDriver[driver] = channel
        if (channelPerDriver[key] !== undefined)
            channel = channelPerDriver[key]
        else if (drivers[index].defaultChannel !== "")
            channel = drivers[index].defaultChannel
        driver = key
    }

    function start() {
        bridge.startServer(driver, channel, tcNumber, booms, sections, channels)
    }

    function applyStartupOptions(options) {
        if (indexOfDriver(options.driver) >= 0)
            selectDriver(options.driver)
        if (options.channel !== undefined) channel = options.channel
        if (options["tc-number"] !== undefined) tcNumber = options["tc-number"]
        if (options.booms !== undefined) booms = options.booms
        if (options.sections !== undefined) sections = options.sections
        if (options.channels !== undefined) channels = options.channels
        if (options.autostart) Qt.callLater(start)
    }
}
