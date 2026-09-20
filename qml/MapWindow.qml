import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Detached field-operation window: 2D map, section strip, and field drawing controls.
Window {
    id: root
    visible: false
    width: 1100
    height: 750
    minimumWidth: 800
    minimumHeight: 600
    title: "Field operation map"
    color: "#1b1e24"

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 8

        FieldMapView {
            id: fieldMap
            Layout.fillWidth: true
            Layout.fillHeight: true
            drawMode: drawModeBox.checked
            nudgeStepM: Number(nudgeStepBox.currentText)
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            SectionStatusPanel {
                Layout.fillWidth: true
            }

            GroupBox {
                title: "Draw field"
                RowLayout {
                    anchors.fill: parent
                    CheckBox {
                        id: drawModeBox
                        text: "Draw"
                        checked: true
                    }
                    Label { text: "Nudge m"; color: "#c7d0dc" }
                    ComboBox {
                        id: nudgeStepBox
                        model: ["0.01", "0.05", "0.1", "0.5", "1", "5"]
                        currentIndex: 2
                        Layout.preferredWidth: 82
                    }
                    Button {
                        text: "Create field"
                        enabled: fieldMap.draftPoints.length >= 3
                        onClicked: {
                            if (bridge.createFieldFromLocalBoundary(fieldNameField.text, fieldMap.draftPoints))
                                fieldMap.clearDraft()
                        }
                    }
                    Button {
                        text: "Clear"
                        enabled: fieldMap.draftPoints.length > 0
                        onClicked: fieldMap.clearDraft()
                    }
                    TextField {
                        id: fieldNameField
                        text: bridge.fieldNames.length > 0 ? bridge.fieldNames[0] : "Field 1"
                        Layout.preferredWidth: 160
                    }
                }
            }

            GroupBox {
                title: "GPS"
                Label {
                    text: bridge.gpsValid
                          ? bridge.gpsLatitude.toFixed(7) + ", " + bridge.gpsLongitude.toFixed(7)
                            + "  " + bridge.gpsSpeedKph.toFixed(1) + " km/h"
                          : "Waiting for position (" + bridge.gpsSourceText + ")"
                    color: bridge.gpsValid ? "#dce8f5" : "#8995a3"
                }
            }
        }
    }
}
