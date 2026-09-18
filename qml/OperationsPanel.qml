import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

GroupBox {
    title: "GPS, field and task"

    ScrollView {
        anchors.fill: parent
        clip: true
        contentWidth: availableWidth

        ColumnLayout {
            width: parent.width
            spacing: 8

            Label {
                text: "GPS source"
                color: "#73c7ff"
                font.bold: true
            }

            GridLayout {
                Layout.fillWidth: true
                columns: 4
                columnSpacing: 6
                rowSpacing: 5

                Label { text: "Source"; color: "#c7d0dc" }
                ComboBox {
                    id: sourceBox
                    model: ["Simulated", "NMEA serial", "ISO CAN", "Auto"]
                    enabled: !bridge.gpsRunning
                    Layout.columnSpan: 3
                    Layout.fillWidth: true
                }

                Label { text: "Port"; color: "#c7d0dc" }
                TextField {
                    id: portField
                    placeholderText: "COM4"
                    enabled: !bridge.gpsRunning && (sourceBox.currentText === "NMEA serial" || sourceBox.currentText === "Auto")
                    Layout.fillWidth: true
                }
                Label { text: "Baud"; color: "#c7d0dc" }
                ComboBox {
                    id: baudBox
                    model: [4800, 9600, 38400, 115200]
                    currentIndex: 3
                    enabled: !bridge.gpsRunning
                    Layout.fillWidth: true
                }

                Label { text: "Start lat"; color: "#c7d0dc" }
                TextField {
                    id: latitudeField
                    text: "52.000000"
                    enabled: !bridge.gpsRunning && sourceBox.currentText === "Simulated"
                    validator: DoubleValidator { bottom: -90; top: 90; decimals: 8 }
                    Layout.fillWidth: true
                }
                Label { text: "Start lon"; color: "#c7d0dc" }
                TextField {
                    id: longitudeField
                    text: "5.000000"
                    enabled: !bridge.gpsRunning && sourceBox.currentText === "Simulated"
                    validator: DoubleValidator { bottom: -180; top: 180; decimals: 8 }
                    Layout.fillWidth: true
                }
            }

            Button {
                Layout.fillWidth: true
                text: bridge.gpsRunning ? "Stop GPS" : "Start GPS"
                highlighted: !bridge.gpsRunning
                onClicked: {
                    if (bridge.gpsRunning) {
                        bridge.stopGps()
                    } else {
                        bridge.startGps(sourceBox.currentText, portField.text, Number(baudBox.currentText),
                                        Number(latitudeField.text), Number(longitudeField.text))
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                implicitHeight: 64
                radius: 5
                color: "#151a20"
                border.color: bridge.gpsValid ? "#35c759" : "#59636f"
                GridLayout {
                    anchors.fill: parent
                    anchors.margins: 6
                    columns: 2
                    Label {
                        text: bridge.gpsValid ? bridge.gpsLatitude.toFixed(7) + ", " + bridge.gpsLongitude.toFixed(7) : "Waiting for position"
                        color: bridge.gpsValid ? "#dce8f5" : "#8995a3"
                    }
                    Label {
                        text: bridge.gpsSourceText
                        color: "#73c7ff"
                        horizontalAlignment: Text.AlignRight
                        Layout.fillWidth: true
                    }
                    Label { text: bridge.gpsSpeedKph.toFixed(1) + " km/h"; color: "#c7d0dc" }
                    Label {
                        text: bridge.gpsCourse.toFixed(1) + "°"
                        color: "#c7d0dc"
                        horizontalAlignment: Text.AlignRight
                        Layout.fillWidth: true
                    }
                }
            }

            GridLayout {
                Layout.fillWidth: true
                columns: 4
                enabled: bridge.gpsRunning && bridge.gpsSourceText === "Simulated"
                Label { text: "Speed"; color: "#c7d0dc" }
                SpinBox {
                    id: speedSpin
                    from: 0
                    to: 50
                    value: 8
                    editable: true
                    Layout.fillWidth: true
                }
                Label { text: "Heading"; color: "#c7d0dc" }
                SpinBox {
                    id: courseSpin
                    from: 0
                    to: 359
                    value: 0
                    editable: true
                    Layout.fillWidth: true
                }
                Button {
                    text: "↶ 5°"
                    onClicked: {
                        courseSpin.value = (courseSpin.value + 355) % 360
                        bridge.nudgeSimulation(0, -5)
                    }
                }
                Button {
                    text: "▲ 1 m"
                    onClicked: bridge.nudgeSimulation(1, 0)
                }
                Button {
                    text: "↷ 5°"
                    onClicked: {
                        courseSpin.value = (courseSpin.value + 5) % 360
                        bridge.nudgeSimulation(0, 5)
                    }
                }
                Button {
                    text: "Drive"
                    highlighted: true
                    onClicked: bridge.setSimulationMotion(speedSpin.value, courseSpin.value)
                }
            }

            Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: "#3b4652" }
            Label { text: "Field"; color: "#73c7ff"; font.bold: true }

            RowLayout {
                Layout.fillWidth: true
                ComboBox {
                    id: fieldBox
                    Layout.fillWidth: true
                    model: bridge.fieldNames
                    currentIndex: bridge.selectedFieldIndex
                    displayText: count > 0 && currentIndex >= 0 ? currentText : "No field defined"
                    onActivated: bridge.selectField(currentIndex)
                }
            }
            GridLayout {
                Layout.fillWidth: true
                columns: 4
                Label { text: "Name"; color: "#c7d0dc" }
                TextField { id: fieldName; text: "Field 1"; Layout.columnSpan: 3; Layout.fillWidth: true }
                Label { text: "Width m"; color: "#c7d0dc" }
                SpinBox { id: fieldWidth; from: 1; to: 10000; value: 200; editable: true; Layout.fillWidth: true }
                Label { text: "Length m"; color: "#c7d0dc" }
                SpinBox { id: fieldLength; from: 1; to: 10000; value: 300; editable: true; Layout.fillWidth: true }
            }
            Button {
                Layout.fillWidth: true
                text: "Create field at GPS position"
                enabled: bridge.gpsValid
                onClicked: bridge.createField(fieldName.text, fieldWidth.value, fieldLength.value)
            }

            Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: "#3b4652" }
            Label { text: "Task"; color: "#73c7ff"; font.bold: true }
            ComboBox {
                id: taskBox
                Layout.fillWidth: true
                model: bridge.taskNames
                currentIndex: bridge.selectedTaskIndex
                displayText: count > 0 && currentIndex >= 0 ? currentText : "No task defined"
                onActivated: bridge.selectTask(currentIndex)
            }
            RowLayout {
                Layout.fillWidth: true
                TextField { id: taskName; text: "Task 1"; Layout.fillWidth: true }
                Button {
                    text: "Create"
                    enabled: bridge.selectedFieldIndex >= 0
                    onClicked: bridge.createTask(taskName.text)
                }
            }
            RowLayout {
                Layout.fillWidth: true
                Button {
                    text: "Start"
                    highlighted: !bridge.taskActive
                    enabled: bridge.selectedTaskIndex >= 0 && !bridge.taskActive
                    onClicked: bridge.startSelectedTask()
                }
                Button {
                    text: "Pause"
                    enabled: bridge.taskActive
                    onClicked: bridge.pauseSelectedTask()
                }
                Button {
                    text: "Complete"
                    enabled: bridge.selectedTaskIndex >= 0
                    onClicked: bridge.stopSelectedTask()
                }
                Button {
                    text: "Clear track"
                    onClicked: bridge.clearTrack()
                }
            }

            Label {
                Layout.fillWidth: true
                text: bridge.taskActive ? "Recording: " + bridge.activeTaskName + " / " + bridge.activeFieldName : "No active task"
                color: bridge.taskActive ? "#35c759" : "#8995a3"
                elide: Text.ElideRight
            }
        }
    }
}
