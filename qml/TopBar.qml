import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

RowLayout {
    id: topBar
    spacing: 8

    signal openTaskDataRequested()

    // Channel text per driver, so switching drivers keeps each bus/network name.
    // PCAN Virtual defaults to the CAN-API 2 network name AgIsoVirtualTerminal uses.
    property var channelPerDriver: ({ "pcan_virtual": "PCANLight_USB" })

    function startServer() {
        bridge.startServer(driverBox.currentText, channelField.text, tcSpin.value, boomSpin.value, sectionSpin.value, channelSpin.value);
    }

    // Command-line options (--driver, --channel, --tc-number, ...) preset the controls, and
    // --autostart presses Start once everything is set.
    Component.onCompleted: {
        const driverIndex = driverBox.model.indexOf(startupOptions.driver);
        if (driverIndex >= 0) {
            driverBox.currentIndex = driverIndex;
            driverBox.previousDriver = startupOptions.driver;
            if (channelPerDriver[startupOptions.driver] !== undefined) {
                channelField.text = channelPerDriver[startupOptions.driver];
            }
        }
        if (startupOptions.channel !== undefined) channelField.text = startupOptions.channel;
        if (startupOptions["tc-number"] !== undefined) tcSpin.value = startupOptions["tc-number"];
        if (startupOptions.booms !== undefined) boomSpin.value = startupOptions.booms;
        if (startupOptions.sections !== undefined) sectionSpin.value = startupOptions.sections;
        if (startupOptions.channels !== undefined) channelSpin.value = startupOptions.channels;
        if (startupOptions.autostart) Qt.callLater(startServer);
    }

    Label {
        text: "Driver:"
        color: "#c7d0dc"
    }
    ComboBox {
        id: driverBox
        property string previousDriver: ""
        model: ["wcan", "pcan_usb", "pcan_virtual", "virtual", "socketcan"]
        enabled: !bridge.running
        Layout.preferredWidth: 130
        Component.onCompleted: previousDriver = currentText
        onActivated: {
            topBar.channelPerDriver[previousDriver] = channelField.text;
            if (topBar.channelPerDriver[currentText] !== undefined) {
                channelField.text = topBar.channelPerDriver[currentText];
            }
            previousDriver = currentText;
        }
    }
    Label {
        text: "Channel:"
        color: "#c7d0dc"
    }
    TextField {
        id: channelField
        text: "big_planter_isobus"
        enabled: !bridge.running
        Layout.preferredWidth: 130
    }
    Label {
        text: "TC #:"
        color: "#c7d0dc"
    }
    SpinBox {
        id: tcSpin
        from: 1
        to: 32
        value: 1
        enabled: !bridge.running
        Layout.preferredWidth: 80
    }
    Label {
        text: "Booms:"
        color: "#c7d0dc"
    }
    SpinBox {
        id: boomSpin
        from: 1
        to: 255
        value: 4
        enabled: !bridge.running
        Layout.preferredWidth: 80
    }
    Label {
        text: "Sections:"
        color: "#c7d0dc"
    }
    SpinBox {
        id: sectionSpin
        from: 1
        to: 255
        value: 64
        enabled: !bridge.running
        Layout.preferredWidth: 80
    }
    Label {
        text: "Channels:"
        color: "#c7d0dc"
    }
    SpinBox {
        id: channelSpin
        from: 0
        to: 255
        value: 16
        enabled: !bridge.running
        Layout.preferredWidth: 80
    }
    Button {
        text: bridge.running ? "Stop server" : "Start server"
        highlighted: !bridge.running
        onClicked: {
            if (bridge.running) {
                bridge.stopServer();
            } else {
                topBar.startServer();
            }
        }
    }
    Button {
        text: bridge.taskActive ? "Stop task" : "Start task"
        enabled: bridge.selectedTaskIndex >= 0
        onClicked: bridge.setTaskActive(!bridge.taskActive)
    }
    Button {
        text: "TC data"
        onClicked: openTaskDataRequested()
    }
    Item {
        Layout.fillWidth: true
    }
}
