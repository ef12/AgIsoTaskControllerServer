import QtQuick
import QtQuick.Layouts
import AgIsoTc 1.0
import "../components"

// The GPS source and the live position. The simulated tractor is driven from the 3D view.
ScrollPage {
    id: page

    readonly property var sources: [
        { name: "Simulated", hint: "A tractor you drive yourself from the 3D view, starting at the position below." },
        { name: "NMEA serial", hint: "An NMEA 0183 receiver on a serial port." },
        { name: "ISO CAN", hint: "Position messages from the CAN bus (NMEA 2000 / ISOBUS)." },
        { name: "Auto", hint: "The serial receiver when a port is given, and position messages from the CAN bus." }
    ]
    readonly property string source: sources[sourceBox.currentIndex].name
    readonly property bool simulated: bridge.gpsRunning && bridge.gpsSourceText === "Simulated"

    component KeyHint: RowLayout {
        id: hint
        property var keys: []
        property string action
        spacing: 4
        Repeater {
            model: hint.keys
            delegate: KeyCap { text: modelData }
        }
        Text {
            Layout.leftMargin: 4
            text: hint.action
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSmall
            color: Theme.textSecondary
        }
    }

    PageHeader {
        Layout.fillWidth: true
        title: "GPS & driving"
        subtitle: "Where the tractor is, and where its position comes from"
    }

    // --- live position ---------------------------------------------------------------------
    Card {
        Layout.fillWidth: true

        RowLayout {
            Layout.fillWidth: true
            spacing: 14

            CompassDial {
                size: 64
                course: bridge.gpsCourse
                active: bridge.gpsValid
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 4
                RowLayout {
                    spacing: 8
                    StatusBadge {
                        text: bridge.gpsValid ? "Position fix" : (bridge.gpsRunning ? "Searching" : "GPS off")
                        tone: bridge.gpsValid ? "success" : (bridge.gpsRunning ? "warning" : "neutral")
                        pulse: bridge.gpsRunning && !bridge.gpsValid
                    }
                    Text {
                        visible: bridge.gpsRunning
                        text: bridge.gpsSourceText
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontCaption
                        color: Theme.textMuted
                    }
                }
                Text {
                    Layout.fillWidth: true
                    text: bridge.gpsValid ? bridge.gpsLatitude.toFixed(7) + ",  " + bridge.gpsLongitude.toFixed(7)
                                          : "No position yet"
                    font.family: bridge.gpsValid ? Theme.monoFamily : Theme.fontFamily
                    font.pixelSize: Theme.fontBody
                    color: bridge.gpsValid ? Theme.text : Theme.textMuted
                    elide: Text.ElideRight
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 0
            Metric {
                Layout.fillWidth: true
                label: "Speed"
                iconName: "gauge"
                value: bridge.gpsSpeedKph.toFixed(1)
                unit: "km/h"
                valueColor: bridge.gpsValid ? Theme.text : Theme.textMuted
            }
            Metric {
                Layout.fillWidth: true
                label: "Course"
                iconName: "compass"
                value: bridge.gpsCourse.toFixed(0) + "°"
                valueColor: bridge.gpsValid ? Theme.text : Theme.textMuted
            }
        }
    }

    // --- source ------------------------------------------------------------------------------
    Card {
        Layout.fillWidth: true
        title: "Source"
        subtitle: bridge.gpsRunning ? "Stop the GPS to change the source." : ""
        iconName: "locate"

        FormField {
            Layout.fillWidth: true
            label: "Position from"
            hint: page.sources[sourceBox.currentIndex].hint
            AppComboBox {
                id: sourceBox
                Layout.fillWidth: true
                enabled: !bridge.gpsRunning
                model: page.sources
                textRole: "name"
            }
        }

        RowLayout {
            visible: page.source === "NMEA serial" || page.source === "Auto"
            Layout.fillWidth: true
            spacing: 12
            FormField {
                Layout.fillWidth: true
                label: "Serial port"
                AppTextField {
                    id: portField
                    Layout.fillWidth: true
                    placeholderText: "COM4"
                    enabled: !bridge.gpsRunning
                }
            }
            FormField {
                Layout.preferredWidth: 120
                label: "Baud rate"
                AppComboBox {
                    id: baudBox
                    Layout.fillWidth: true
                    model: [4800, 9600, 38400, 115200]
                    currentIndex: 3
                    enabled: !bridge.gpsRunning
                }
            }
        }

        RowLayout {
            visible: page.source === "Simulated"
            Layout.fillWidth: true
            spacing: 12
            FormField {
                Layout.fillWidth: true
                label: "Start latitude"
                AppTextField {
                    id: latitudeField
                    Layout.fillWidth: true
                    text: "52.000000"
                    enabled: !bridge.gpsRunning
                    // the coordinates take a decimal point whatever the system locale
                    validator: DoubleValidator { locale: "C"; bottom: -90; top: 90; decimals: 8 }
                }
            }
            FormField {
                Layout.fillWidth: true
                label: "Start longitude"
                AppTextField {
                    id: longitudeField
                    Layout.fillWidth: true
                    text: "5.000000"
                    enabled: !bridge.gpsRunning
                    validator: DoubleValidator { locale: "C"; bottom: -180; top: 180; decimals: 8 }
                }
            }
        }

        AppButton {
            Layout.fillWidth: true
            size: "lg"
            text: bridge.gpsRunning ? "Stop GPS" : "Start GPS"
            iconName: bridge.gpsRunning ? "stop" : "play"
            variant: bridge.gpsRunning ? "dangerSoft" : "primary"
            onClicked: {
                if (bridge.gpsRunning) {
                    bridge.stopGps()
                } else {
                    bridge.startGps(page.source, portField.text, Number(baudBox.currentText),
                                    Number(latitudeField.text), Number(longitudeField.text))
                }
            }
        }
    }

    // --- driving -----------------------------------------------------------------------------
    Card {
        Layout.fillWidth: true
        title: "Driving"
        subtitle: page.simulated ? "Use the drive pad in the 3D view, or click the view and drive with the keyboard."
                                 : "Start the simulated GPS to drive the tractor yourself."
        iconName: "steering"
        iconColor: page.simulated ? Theme.accentText : Theme.textMuted

        GridLayout {
            visible: page.simulated
            Layout.fillWidth: true
            columns: 2
            columnSpacing: 12
            rowSpacing: 8

            KeyHint { keys: ["W", "S"]; action: "Throttle" }
            KeyHint { keys: ["A", "D"]; action: "Steer" }
            KeyHint { keys: ["Space"]; action: "Stop" }
            KeyHint { keys: ["C"]; action: "Centre wheel" }
        }
    }
}
