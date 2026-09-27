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
        to: 96
        value: 16
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
                bridge.startServer(driverBox.currentText, channelField.text, tcSpin.value, boomSpin.value, sectionSpin.value, channelSpin.value);
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
