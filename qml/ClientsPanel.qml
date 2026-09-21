import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

GroupBox {
    title: "TC clients"
    ColumnLayout {
        anchors.fill: parent
        spacing: 6
        ListView {
            id: clientList
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: clientModel
            clip: true
            delegate: Rectangle {
                width: clientList.width
                height: 62
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
                    anchors.margins: 6
                    spacing: 2
                    Text {
                        text: "Addr " + clientAddress + "   " + clientName + (timedOut ? "   (TIMEOUT)" : "")
                        color: "#e8edf3"
                        font.bold: true
                        font.pixelSize: 13
                    }
                    Text {
                        text: "Func " + clientFunction + "   Mfr " + manufacturerCode + "   DDOP " + ddopSize + " B" + (ddopActive ? " ACTIVE" : "")
                        color: "#9fb0c3"
                        font.pixelSize: 12
                    }
                }
            }
        }
        Label {
            text: "Heard on bus (address claims, connected or not)"
            color: "#8995a3"
            font.pixelSize: 11
            font.bold: true
        }
        ListView {
            id: peerList
            Layout.fillWidth: true
            Layout.preferredHeight: Math.min(110, count * 24 + 4)
            model: bridge.busPeers
            clip: true
            delegate: Text {
                width: peerList.width
                text: "Addr " + modelData.address + "   func " + modelData.functionCode + "/" + modelData.functionInstance
                      + "   mfr " + modelData.manufacturerCode + (modelData.connected ? "   CONNECTED" : "   not connected")
                color: modelData.connected ? "#8fe388" : "#f2d33c"
                font.pixelSize: 12
                elide: Text.ElideRight
            }
        }
        RowLayout {
            spacing: 6
            Button {
                text: "Load pool file"
                enabled: bridge.selectedClient >= 0
                onClicked: poolDialog.open()
            }
            Button {
                text: "Clear pool"
                enabled: bridge.selectedClient >= 0
                onClicked: bridge.clearPool()
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
