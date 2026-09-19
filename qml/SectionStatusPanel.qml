import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

GroupBox {
    id: root
    title: "TC-SC Section Control"
    width: 300
    Layout.fillHeight: true

    property var sections: []

    ColumnLayout {
        anchors.fill: parent
        spacing: 8

        // Header
        RowLayout {
            Layout.fillWidth: true
            Label {
                text: "Section"
                color: "#90A4AE"
                font.bold: true
                font.pixelSize: 12
                Layout.fillWidth: true
            }
            Label {
                text: "Status"
                color: "#90A4AE"
                font.bold: true
                font.pixelSize: 12
                Layout.alignment: Qt.AlignRight
            }
            Label {
                text: "Area (ha)"
                color: "#90A4AE"
                font.bold: true
                font.pixelSize: 12
                Layout.alignment: Qt.AlignRight
            }
        }

        // Section list
        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: root.sections
            clip: true
            spacing: 4
            delegate: RowLayout {
                Layout.fillWidth: true
                height: 40

                // Section number
                Label {
                    text: "S" + (index + 1)
                    color: "#ECEFF1"
                    font.pixelSize: 14
                    font.bold: true
                    Layout.preferredWidth: 40
                }

                // Status indicator
                Rectangle {
                    width: 16
                    height: 16
                    radius: 8
                    color: modelData.isOn ? "#4CAF50" : "#757575"
                    border.color: modelData.isOverlapping ? "#FFC107" : "transparent"
                    border.width: modelData.isOverlapping ? 2 : 0
                    Layout.alignment: Qt.AlignLeft
                }

                // Status text
                Label {
                    text: modelData.isOn ? "ON" : "OFF"
                    color: modelData.isOn ? "#81C784" : "#90A4AE"
                    font.pixelSize: 13
                    font.bold: true
                    Layout.fillWidth: true
                }

                // Overlap indicator
                Label {
                    text: modelData.isOverlapping ? "⚠ OVERLAP" : ""
                    color: "#FFC107"
                    font.pixelSize: 11
                    Layout.alignment: Qt.AlignRight
                }

                // Worked area
                Label {
                    text: modelData.workedAreaHectares.toFixed(2) + " ha"
                    color: "#90A4AE"
                    font.pixelSize: 12
                    Layout.alignment: Qt.AlignRight
                    Layout.preferredWidth: 70
                }
            }
        }

        // Summary
        Rectangle {
            Layout.fillWidth: true
            height: 60
            color: "#1E2730"
            radius: 8
            border.color: "#37474F"
            border.width: 1

            GridLayout {
                anchors.fill: parent
                anchors.margins: 12
                columns: 3
                rows: 2

                Label {
                    text: "Total Sections"
                    color: "#90A4AE"
                    font.pixelSize: 11
                }
                Label {
                    text: root.sections.length
                    color: "#ECEFF1"
                    font.pixelSize: 18
                    font.bold: true
                    Layout.column: 1
                }

                Label {
                    text: "Active"
                    color: "#90A4AE"
                    font.pixelSize: 11
                }
                Label {
                    text: root.sections.reduce(function(sum, s) { return sum + (s.isOn ? 1 : 0); }, 0)
                    color: "#4CAF50"
                    font.pixelSize: 18
                    font.bold: true
                    Layout.column: 1
                }

                Label {
                    text: "Overlapping"
                    color: "#90A4AE"
                    font.pixelSize: 11
                }
                Label {
                    text: root.sections.reduce(function(sum, s) { return sum + (s.isOverlapping ? 1 : 0); }, 0)
                    color: "#FFC107"
                    font.pixelSize: 18
                    font.bold: true
                    Layout.column: 1
                }

                Label {
                    text: "Total Worked"
                    color: "#90A4AE"
                    font.pixelSize: 11
                    Layout.columnSpan: 2
                }
                Label {
                    text: root.sections.reduce(function(sum, s) { return sum + s.workedAreaHectares; }, 0).toFixed(2) + " ha"
                    color: "#2196F3"
                    font.pixelSize: 18
                    font.bold: true
                    Layout.column: 1
                }
            }
        }
    }
}