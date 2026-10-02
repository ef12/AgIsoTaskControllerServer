import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

// The TC clients and the device descriptor of the selected one, as master and detail: the
// client list on top (click a client to select it), the devices heard on the bus folded into one
// line, and below them the selected client's DDOP (its object tree and its declared DDIs) with the
// pool actions, which act on that client.
GroupBox {
    id: root
    title: "TC clients and DDOP"

    property bool peersOpen: false
    readonly property bool clientSelected: bridge.selectedClient >= 0
    readonly property int connectedPeers: {
        let count = 0
        for (let i = 0; i < bridge.busPeers.length; ++i) count += bridge.busPeers[i].connected ? 1 : 0
        return count
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 6

        // --- the clients ---------------------------------------------------------------------
        Label {
            visible: clientList.count === 0
            Layout.fillWidth: true
            text: "No TC client yet: waiting for an implement to connect."
            color: "#8995a3"
            font.pixelSize: 12
            wrapMode: Text.Wrap
        }
        ListView {
            id: clientList
            visible: count > 0
            Layout.fillWidth: true
            Layout.preferredHeight: Math.min(3, count) * 52
            model: clientModel
            clip: true
            spacing: 4
            delegate: Rectangle {
                width: clientList.width
                height: 48
                color: clientAddress === bridge.selectedClient ? "#2f4a63" : (index % 2 ? "#22262e" : "#262b34")
                border.color: ddopActive ? "#3fae5a" : "#555c66"
                border.width: 1
                radius: 4
                MouseArea {
                    anchors.fill: parent
                    onClicked: bridge.selectClient(clientAddress)
                }
                Column {
                    anchors.fill: parent
                    anchors.margins: 5
                    spacing: 1
                    Text {
                        width: parent.width
                        text: "Addr " + clientAddress + "   " + clientName + (timedOut ? "   (TIMEOUT)" : "")
                        color: "#e8edf3"
                        font.bold: true
                        font.pixelSize: 13
                        elide: Text.ElideRight
                    }
                    Text {
                        width: parent.width
                        text: "Func " + clientFunction + "   Mfr " + manufacturerCode + "   DDOP " + ddopSize + " B" + (ddopActive ? " ACTIVE" : "")
                        color: "#9fb0c3"
                        font.pixelSize: 12
                        elide: Text.ElideRight
                    }
                }
            }
        }

        // --- the devices heard on the bus, folded into one line -------------------------------
        Label {
            Layout.fillWidth: true
            text: (root.peersOpen ? "▾" : "▸") + "  Heard on bus: " + bridge.busPeers.length
                  + " (" + root.connectedPeers + " connected)"
            color: "#8995a3"
            font.pixelSize: 11
            font.bold: true
            MouseArea {
                id: peerMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: root.peersOpen = !root.peersOpen
            }
            ToolTip.visible: peerMouse.containsMouse
            ToolTip.text: "Address claims seen on the bus, connected to this TC or not. Click to show or hide them."
        }
        ListView {
            id: peerList
            visible: root.peersOpen
            Layout.fillWidth: true
            Layout.preferredHeight: Math.min(110, count * 20 + 4)
            model: bridge.busPeers
            clip: true
            delegate: Text {
                width: peerList.width
                leftPadding: 14
                text: "Addr " + modelData.address + "   func " + modelData.functionCode + "/" + modelData.functionInstance
                      + "   mfr " + modelData.manufacturerCode + (modelData.connected ? "   CONNECTED" : "   not connected")
                color: modelData.connected ? "#8fe388" : "#f2d33c"
                font.pixelSize: 12
                elide: Text.ElideRight
            }
        }

        Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: "#3a4048" }

        // --- the selected client's DDOP ------------------------------------------------------
        RowLayout {
            Layout.fillWidth: true
            spacing: 6
            Label {
                Layout.fillWidth: true
                text: root.clientSelected ? "DDOP of client " + bridge.selectedClient : "DDOP"
                color: "#dfe6ee"
                font.bold: true
                elide: Text.ElideRight
            }
            Button {
                text: "Load pool file"
                enabled: root.clientSelected
                onClicked: poolDialog.open()
                ToolTip.visible: hovered
                ToolTip.text: "Use a DDOP binary (.bin, .iop) for the selected client"
            }
            Button {
                text: "Clear pool"
                enabled: root.clientSelected
                onClicked: bridge.clearPool()
            }
        }
        Label {
            visible: !root.clientSelected
            Layout.fillWidth: true
            text: "Select a client above to see its device descriptor."
            color: "#8995a3"
            font.pixelSize: 12
            wrapMode: Text.Wrap
        }

        // keeps the lines above at the top while there is no DDOP to fill the panel
        Item {
            visible: !root.clientSelected
            Layout.fillHeight: true
        }

        TabBar {
            id: tabs
            visible: root.clientSelected
            Layout.fillWidth: true
            TabButton { text: "Objects" }
            TabButton { text: "Declared DDIs (" + bridge.implementDdis.length + ")" }
        }

        StackLayout {
            visible: root.clientSelected
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: tabs.currentIndex

            ListView {
                id: ddopList
                model: ddopModel
                clip: true
                delegate: Text {
                    width: ddopList.width
                    leftPadding: 6 + indent * 18
                    topPadding: 2
                    bottomPadding: 2
                    color: indent === 0 ? "#7fd0ff" : "#dfe6ee"
                    font.pixelSize: indent === 0 ? 13 : 12
                    font.bold: indent === 0
                    wrapMode: Text.Wrap
                    text: rowText
                }
            }

            ColumnLayout {
                spacing: 4
                RowLayout {
                    Layout.fillWidth: true
                    CheckBox {
                        text: "Auto sync"
                        ToolTip.visible: hovered
                        ToolTip.text: "Request the declared DDIs one after the other, at this interval"
                        checked: bridge.autoDdiSync
                        onToggled: bridge.setAutoDdiSync(checked)
                    }
                    Label { text: "Interval"; color: "#9fb0c3" }
                    SpinBox {
                        from: 250
                        to: 60000
                        stepSize: 250
                        value: bridge.ddiSyncIntervalMs
                        editable: true
                        Layout.preferredWidth: 100
                        onValueModified: bridge.setDdiSyncIntervalMs(value)
                    }
                    Button { text: "Request all"; onClicked: bridge.requestImplementDdis() }
                }
                Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: "#3a4048" }
                ListView {
                    id: ddiList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    model: bridge.implementDdis
                    clip: true
                    delegate: Rectangle {
                        width: ddiList.width
                        height: 42
                        color: index % 2 ? "#151a20" : "transparent"
                        Column {
                            anchors.fill: parent
                            anchors.leftMargin: 6
                            Text {
                                text: "DDI " + modelData.ddi + "  element " + modelData.element + "  " + modelData.name
                                color: "#dfe6ee"
                                font.pixelSize: 12
                            }
                            Text {
                                text: (modelData.hasValue ? "Value " + modelData.value + "  " + modelData.updated : "Waiting for value")
                                      + "  •  " + modelData.triggers + (modelData.settable ? "  •  settable" : "")
                                color: modelData.hasValue ? "#8fe388" : "#9fb0c3"
                                font.pixelSize: 11
                            }
                        }
                    }
                }
            }
        }
    }

    FileDialog {
        id: poolDialog
        title: "Open DDOP binary"
        nameFilters: ["DDOP files (*.bin *.iop *.ddop)", "All files (*)"]
        onAccepted: bridge.loadPoolFile(selectedFile)
    }
}
