import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

GroupBox {
    title: "GPS, field and task"

    signal openFieldMapRequested()

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
                text: "Open field map"
                enabled: bridge.gpsRunning
                onClicked: openFieldMapRequested()
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

            Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: "#3b4652" }
            Label { text: "Driving controls"; color: "#73c7ff"; font.bold: true }

            ColumnLayout {
                Layout.fillWidth: true
                enabled: bridge.gpsRunning && bridge.gpsSourceText === "Simulated"
                RowLayout {
                    Layout.alignment: Qt.AlignHCenter
                    Item {
                        Layout.preferredWidth: 142
                        Layout.preferredHeight: 142
                        Dial {
                            id: drivingWheel
                            anchors.fill: parent
                            from: -40
                            to: 40
                            value: bridge.steeringAngle
                            stepSize: 1
                            snapMode: Dial.SnapAlways
                            wrap: false
                            onMoved: bridge.setSteeringAngle(value)
                            onValueChanged: wheelCanvas.requestPaint()
                            background: Canvas {
                                id: wheelCanvas
                                anchors.fill: parent
                                onPaint: {
                                    var ctx = getContext("2d");
                                    var w = width, h = height;
                                    var cx = w / 2, cy = h / 2;
                                    var radius = Math.min(w, h) / 2 - 10;
                                    ctx.clearRect(0, 0, w, h);
                                    ctx.save();
                                    ctx.translate(cx, cy);
                                    ctx.rotate(drivingWheel.value * 2.4 * Math.PI / 180);

                                    var rim = ctx.createRadialGradient(0, 0, radius * 0.72, 0, 0, radius);
                                    rim.addColorStop(0, "#303944");
                                    rim.addColorStop(0.62, "#11161b");
                                    rim.addColorStop(1, drivingWheel.pressed ? "#73c7ff" : "#050709");
                                    ctx.lineWidth = 13;
                                    ctx.strokeStyle = rim;
                                    ctx.beginPath();
                                    ctx.arc(0, 0, radius, 0, Math.PI * 2);
                                    ctx.stroke();

                                    ctx.lineWidth = 3;
                                    ctx.strokeStyle = "#5d6975";
                                    ctx.beginPath();
                                    ctx.arc(0, 0, radius - 7, 0, Math.PI * 2);
                                    ctx.stroke();

                                    ctx.strokeStyle = "#aeb8c4";
                                    ctx.lineWidth = 8;
                                    ctx.lineCap = "round";
                                    for (var i = 0; i < 3; ++i) {
                                        var angle = (-90 + i * 120) * Math.PI / 180;
                                        ctx.beginPath();
                                        ctx.moveTo(Math.cos(angle) * 14, Math.sin(angle) * 14);
                                        ctx.lineTo(Math.cos(angle) * (radius - 18), Math.sin(angle) * (radius - 18));
                                        ctx.stroke();
                                    }

                                    ctx.fillStyle = "#1d2630";
                                    ctx.strokeStyle = "#73c7ff";
                                    ctx.lineWidth = 2;
                                    ctx.beginPath();
                                    ctx.arc(0, 0, 18, 0, Math.PI * 2);
                                    ctx.fill();
                                    ctx.stroke();

                                    ctx.strokeStyle = "#dfe6ee";
                                    ctx.lineWidth = 2;
                                    for (var tick = 0; tick < 12; ++tick) {
                                        var tickAngle = tick * 30 * Math.PI / 180;
                                        ctx.beginPath();
                                        ctx.moveTo(Math.cos(tickAngle) * (radius - 5), Math.sin(tickAngle) * (radius - 5));
                                        ctx.lineTo(Math.cos(tickAngle) * (radius + 1), Math.sin(tickAngle) * (radius + 1));
                                        ctx.stroke();
                                    }
                                    ctx.restore();
                                }
                            }
                            handle: Item { }
                        }
                    }
                    ColumnLayout {
                        Label { text: "Steering"; color: "#c7d0dc"; font.bold: true }
                        Label {
                            text: Math.abs(bridge.steeringAngle) < 0.5 ? "Straight" : Math.abs(bridge.steeringAngle).toFixed(0) + "° " + (bridge.steeringAngle < 0 ? "left" : "right")
                            color: "#73c7ff"
                        }
                        Button { text: "Center wheel"; onClicked: bridge.setSteeringAngle(0) }
                    }
                }

                Label { text: "Constant throttle"; color: "#c7d0dc"; font.bold: true }
                Slider {
                    Layout.fillWidth: true
                    from: 0
                    to: 50
                    stepSize: 0.5
                    value: bridge.throttleKph
                    onMoved: bridge.setThrottleKph(value)
                }
                RowLayout {
                    Layout.fillWidth: true
                    Button { text: "−"; font.pixelSize: 20; onClicked: bridge.adjustThrottle(-1) }
                    Label {
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignHCenter
                        text: bridge.throttleKph.toFixed(1) + " km/h"
                        color: "#8fe388"
                        font.pixelSize: 18
                        font.bold: true
                    }
                    Button { text: "+"; font.pixelSize: 20; onClicked: bridge.adjustThrottle(1) }
                    Button {
                        text: "STOP"
                        font.bold: true
                        palette.button: "#a52a2a"
                        palette.buttonText: "white"
                        onClicked: bridge.stopTractor()
                    }
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

            Button {
                Layout.fillWidth: true
                text: "Draw field boundary"
                onClicked: openFieldMapRequested()
            }

            RowLayout {
                Layout.fillWidth: true
                Button {
                    Layout.fillWidth: true
                    text: "Save fields…"
                    enabled: bridge.fieldNames.length > 0
                    onClicked: saveFieldsDialog.open()
                }
                Button {
                    Layout.fillWidth: true
                    text: "Load fields…"
                    onClicked: loadFieldsDialog.open()
                }
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

            RowLayout {
                Layout.fillWidth: true
                Button {
                    Layout.fillWidth: true
                    text: "Save tasks…"
                    enabled: bridge.taskNames.length > 0
                    onClicked: saveTasksDialog.open()
                }
                Button {
                    Layout.fillWidth: true
                    text: "Load tasks…"
                    onClicked: loadTasksDialog.open()
                }
            }
            Label {
                Layout.fillWidth: true
                text: "Loading tasks keeps their field link; load the fields file first."
                color: "#8995a3"
                font.pixelSize: 11
                wrapMode: Text.Wrap
            }
        }
    }

    FileDialog {
        id: saveFieldsDialog
        title: "Save fields"
        fileMode: FileDialog.SaveFile
        defaultSuffix: "json"
        nameFilters: ["Field files (*.json)", "All files (*)"]
        onAccepted: bridge.saveFields(selectedFile)
    }
    FileDialog {
        id: loadFieldsDialog
        title: "Load fields"
        nameFilters: ["Field files (*.json)", "All files (*)"]
        onAccepted: bridge.loadFields(selectedFile)
    }
    FileDialog {
        id: saveTasksDialog
        title: "Save tasks"
        fileMode: FileDialog.SaveFile
        defaultSuffix: "json"
        nameFilters: ["Task files (*.json)", "All files (*)"]
        onAccepted: bridge.saveTasks(selectedFile)
    }
    FileDialog {
        id: loadTasksDialog
        title: "Load tasks"
        nameFilters: ["Task files (*.json)", "All files (*)"]
        onAccepted: bridge.loadTasks(selectedFile)
    }
}
