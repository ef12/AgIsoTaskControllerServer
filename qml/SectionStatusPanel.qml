import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Compact TC-SC section strip: one block per section (green = ON),
// plus worked totals. Data: bridge.sectionStates (list of bool).
GroupBox {
    title: "Sections"

    ColumnLayout {
        anchors.fill: parent
        spacing: 6

        Label {
            text: bridge.activeSectionCount + " of " + bridge.sectionCount + " sections ON"
            color: bridge.activeSectionCount > 0 ? "#f2d33c" : "#8995a3"
            font.bold: true
            font.pixelSize: 13
        }

        GridLayout {
            Layout.fillWidth: true
            columns: 8
            columnSpacing: 4
            rowSpacing: 4
            Repeater {
                model: bridge.sectionStates
                delegate: Rectangle {
                    Layout.preferredWidth: 22
                    Layout.preferredHeight: 22
                    radius: 4
                    color: modelData ? "#35c759" : "#2b333d"
                    border.color: modelData ? "#7ce796" : "#59636f"
                    border.width: 1
                    Text {
                        anchors.centerIn: parent
                        text: index + 1
                        font.pixelSize: 10
                        color: modelData ? "#0d2812" : "#8995a3"
                    }
                }
            }
        }

        Label {
            text: bridge.workedAreaHa.toFixed(3) + " ha worked"
            color: "#8fe388"
            font.bold: true
            font.pixelSize: 13
        }
    }
}
