import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

GroupBox {
    title: "Event log"
    ColumnLayout {
        anchors.fill: parent
        spacing: 6
        ListView {
            id: logList
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: logModel
            clip: true
            onCountChanged: logList.positionViewAtEnd()
            delegate: Text {
                width: logList.width
                color: "#b9c4d2"
                font.family: "Consolas, monospace"
                font.pixelSize: 11
                wrapMode: Text.Wrap
                text: logLine
            }
        }
        RowLayout {
            Item {
                Layout.fillWidth: true
            }
            Button {
                text: "Clear"
                onClicked: bridge.clearLog()
            }
        }
    }
}
