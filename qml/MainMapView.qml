import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: root
    visible: true
    width: 1600
    height: 1000
    minimumWidth: 1200
    minimumHeight: 800
    title: "AgIso Task Controller - Field Operation"
    color: "#1b1e24"

    // Timer for simulation step
    Timer {
        interval: 50
        running: true
        repeat: true
        onTriggered: {
            bridge.fieldTrackerSimulateStep(50);
        }
    }

    // Poll for bridge updates
    Timer {
        interval: 100
        running: true
        repeat: true
        onTriggered: bridge.poll()
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 8

        // Top bar with connection and basic controls
        TopBar {
            Layout.fillWidth: true
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 8

            // Left panel: Controls
            ColumnLayout {
                Layout.preferredWidth: 320
                Layout.fillHeight: true
                spacing: 8

                // Steering wheel
                GroupBox {
                    title: "Steering Control"
                    Layout.fillWidth: true
                    Layout.preferredHeight: 280

                    SteeringWheel {
                        id: steeringWheel
                        anchors.centerIn: parent
                        value: bridge.fieldTrackerSteering
                        enabled: bridge.fieldTrackerEnabled
                        onValueChanged: bridge.setFieldTrackerSteering(value)
                    }
                }

                // Throttle control
                GroupBox {
                    title: "Throttle Control"
                    Layout.fillWidth: true
                    Layout.preferredHeight: 380

                    ThrottleControl {
                        id: throttleControl
                        anchors.centerIn: parent
                        value: bridge.fieldTrackerThrottle
                        enabled: bridge.fieldTrackerEnabled
                        onValueChanged: bridge.setFieldTrackerThrottle(value)
                        onStopRequested: bridge.fieldTrackerEmergencyStop()
                    }
                }

                // Field boundary recording
                GroupBox {
                    title: "Field Boundary"
                    Layout.fillWidth: true
                    Layout.preferredHeight: 150

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 8

                        RowLayout {
                            TextField {
                                id: fieldNameInput
                                placeholderText: "Field name"
                                Layout.fillWidth: true
                                Layout.preferredHeight: 36
                            }
                        }

                        RowLayout {
                            Button {
                                text: fieldTrackerRecBtn.text
                                highlighted: bridge.fieldTrackerRecording
                                Layout.fillWidth: true
                                Layout.preferredHeight: 40
                                onClicked: {
                                    if (bridge.fieldTrackerRecording) {
                                        bridge.stopFieldBoundaryRecording();
                                    } else {
                                        if (fieldNameInput.text.length > 0) {
                                            bridge.startFieldBoundaryRecording(fieldNameInput.text);
                                        }
                                    }
                                }
                            }

                            Label {
                                id: fieldTrackerRecBtn
                                text: bridge.fieldTrackerRecording ? "Stop Recording" : "Start Recording"
                                color: bridge.fieldTrackerRecording ? "#F44336" : "#4CAF50"
                                font.bold: true
                                font.pixelSize: 13
                            }
                        }

                        Label {
                            text: bridge.fieldTrackerRecording ? "Recording: " + bridge.fieldTrackerCurrentBoundaryVertices + " points" : "Drive around field perimeter to record boundary"
                            color: "#90A4AE"
                            font.pixelSize: 11
                            wrapMode: Text.Wrap
                        }

                        RowLayout {
                            Button {
                                text: "Load Field"
                                Layout.fillWidth: true
                                onClicked: boundaryDialog.open()
                            }
                            Button {
                                text: "Clear Fields"
                                Layout.fillWidth: true
                                onClicked: bridge.clearFields()
                            }
                        }
                    }
                }
            }

            // Center: Map view
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 8

                // Field map view
                GroupBox {
                    title: "Field View"
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    FieldMapView {
                        id: fieldMap
                        anchors.fill: parent
                        latitude: bridge.tractorLatitude
                        longitude: bridge.tractorLongitude
                        heading: bridge.tractorHeading
                        speed: bridge.tractorSpeed
                        trackHistory: bridge.fieldTrackerTrackHistory
                        fieldBoundaries: bridge.fieldBoundaries
                        currentBoundary: bridge.fieldTrackerCurrentBoundary
                        implementLat: bridge.implementLatitude
                        implementLon: bridge.implementLongitude
                        implementHeading: bridge.implementHeading
                        centerLat: bridge.fieldMapCenterLat
                        centerLon: bridge.fieldMapCenterLon
                        zoomLevel: bridge.fieldMapZoom
                        showTrackHistory: bridge.fieldMapShowHistory
                        showFieldBoundaries: bridge.fieldMapShowBoundaries
                    }
                }
            }

            // Right panel: Status panels
            ColumnLayout {
                Layout.preferredWidth: 340
                Layout.fillHeight: true
                spacing: 8

                // TC-Basic data
                GroupBox {
                    title: "TC-Basic Data"
                    Layout.fillWidth: true
                    Layout.preferredHeight: 200

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 4

                        RowLayout {
                            Label { text: "Latitude:"; color: "#90A4AE"; font.pixelSize: 12 }
                            Label { text: bridge.tractorLatitude.toFixed(8); color: "#ECEFF1"; font.bold: true; font.pixelSize: 12; Layout.alignment: Qt.AlignRight }
                        }
                        RowLayout {
                            Label { text: "Longitude:"; color: "#90A4AE"; font.pixelSize: 12 }
                            Label { text: bridge.tractorLongitude.toFixed(8); color: "#ECEFF1"; font.bold: true; font.pixelSize: 12; Layout.alignment: Qt.AlignRight }
                        }
                        RowLayout {
                            Label { text: "Heading:"; color: "#90A4AE"; font.pixelSize: 12 }
                            Label { text: bridge.tractorHeading.toFixed(1) + "°"; color: "#ECEFF1"; font.bold: true; font.pixelSize: 12; Layout.alignment: Qt.AlignRight }
                        }
                        RowLayout {
                            Label { text: "Speed:"; color: "#90A4AE"; font.pixelSize: 12 }
                            Label { text: (bridge.tractorSpeed * 3.6).toFixed(1) + " km/h"; color: "#ECEFF1"; font.bold: true; font.pixelSize: 12; Layout.alignment: Qt.AlignRight }
                        }
                        RowLayout {
                            Label { text: "GPS Fix:"; color: "#90A4AE"; font.pixelSize: 12 }
                            Label { text: bridge.gpsFixQuality; color: bridge.gpsFixValid ? "#4CAF50" : "#F44336"; font.bold: true; font.pixelSize: 12; Layout.alignment: Qt.AlignRight }
                        }
                        RowLayout {
                            Label { text: "Satellites:"; color: "#90A4AE"; font.pixelSize: 12 }
                            Label { text: bridge.gpsSatellites; color: "#ECEFF1"; font.bold: true; font.pixelSize: 12; Layout.alignment: Qt.AlignRight }
                        }
                        RowLayout {
                            Label { text: "Implement Pos:"; color: "#90A4AE"; font.pixelSize: 12 }
                            Label { text: bridge.implementLatitude.toFixed(8) + ", " + bridge.implementLongitude.toFixed(8); color: "#FF9800"; font.bold: true; font.pixelSize: 11; Layout.alignment: Qt.AlignRight }
                        }
                        RowLayout {
                            Label { text: "Implement Heading:"; color: "#90A4AE"; font.pixelSize: 12 }
                            Label { text: bridge.implementHeading.toFixed(1) + "°"; color: "#FF9800"; font.bold: true; font.pixelSize: 12; Layout.alignment: Qt.AlignRight }
                        }
                    }
                }

                // Work tracking
                GroupBox {
                    title: "Work Tracking"
                    Layout.fillWidth: true
                    Layout.preferredHeight: 180

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 8

                        RowLayout {
                            Label { text: "Status:"; color: "#90A4AE"; font.pixelSize: 12 }
                            Label {
                                text: bridge.fieldTrackerWorking ? "WORKING" : "IDLE"
                                color: bridge.fieldTrackerWorking ? "#4CAF50" : "#F44336"
                                font.bold: true
                                font.pixelSize: 16
                                Layout.alignment: Qt.AlignRight
                            }
                        }
                        RowLayout {
                            Label { text: "Field:"; color: "#90A4AE"; font.pixelSize: 12 }
                            Label { text: bridge.fieldTrackerWorkField; color: "#ECEFF1"; font.bold: true; font.pixelSize: 12; Layout.alignment: Qt.AlignRight }
                        }
                        RowLayout {
                            Label { text: "Worked Area:"; color: "#90A4AE"; font.pixelSize: 12 }
                            Label { text: bridge.fieldTrackerWorkedArea.toFixed(2) + " ha"; color: "#2196F3"; font.bold: true; font.pixelSize: 14; Layout.alignment: Qt.AlignRight }
                        }
                        RowLayout {
                            Label { text: "Current Speed:"; color: "#90A4AE"; font.pixelSize: 12 }
                            Label { text: (bridge.fieldTrackerWorkSpeed * 3.6).toFixed(1) + " km/h"; color: "#ECEFF1"; font.bold: true; font.pixelSize: 12; Layout.alignment: Qt.AlignRight }
                        }

                        RowLayout {
                            Button {
                                text: bridge.fieldTrackerWorking ? "Pause Work" : "Start Work"
                                Layout.fillWidth: true
                                highlighted: !bridge.fieldTrackerWorking
                                onClicked: {
                                    if (bridge.fieldTrackerWorking) {
                                        bridge.pauseWork();
                                    } else if (bridge.fieldBoundaries.length > 0) {
                                        bridge.startWork(bridge.fieldBoundaries[0].id);
                                    }
                                }
                            }
                            Button {
                                text: "Stop Work"
                                Layout.fillWidth: true
                                onClicked: bridge.stopWork()
                            }
                        }
                    }
                }

                // TC-SC Section Control
                SectionStatusPanel {
                    sections: bridge.fieldTrackerSections
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                }
            }
        }

        // Status bar
        Label {
            text: bridge.statusText
            color: "#9fb0c3"
            font.pixelSize: 12
        }
    }

    FileDialog {
        id: boundaryDialog
        title: "Select Field Boundary File"
        nameFilters: ["JSON files (*.json)", "All files (*)"]
        onAccepted: bridge.loadFieldBoundary(selectedFile)
    }

    // Identify banner
    Rectangle {
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.topMargin: 56
        width: identifyLabel.implicitWidth + 32
        height: 40
        radius: 8
        color: "#2f4a63"
        border.color: "#5aa2e0"
        visible: identifyTimer.running
        z: 10
        Label {
            id: identifyLabel
            anchors.centerIn: parent
            color: "white"
            font.bold: true
        }
    }

    Timer {
        id: identifyTimer
        interval: 3000
    }

    Connections {
        target: bridge
        function onIdentifyBanner(tcNumber) {
            identifyLabel.text = "TC " + tcNumber + " — identify requested";
            identifyTimer.restart();
        }
    }
}