import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Detached field-operation window: 2D map, section strip, recording controls.
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
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            SectionStatusPanel {
                Layout.fillWidth: true
            }

            GroupBox {
                title: "Perimeter"
                RowLayout {
                    anchors.fill: parent
                    Button {
                        text: bridge.boundaryRecording
                              ? "Recording… " + bridge.boundaryPointCount
                              : "Start perimeter"
                        enabled: bridge.gpsValid && !bridge.boundaryRecording
                        onClicked: bridge.startBoundaryRecording(
                                       bridge.fieldNames.length > 0 ? bridge.fieldNames[0] : "Field 1")
                    }
                    Button {
                        text: "Finish"
                        enabled: bridge.boundaryRecording && bridge.boundaryPointCount >= 3
                        onClicked: bridge.finishBoundaryRecording()
                    }
                    Button {
                        text: "Cancel"
                        enabled: bridge.boundaryRecording
                        onClicked: bridge.cancelBoundaryRecording()
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
