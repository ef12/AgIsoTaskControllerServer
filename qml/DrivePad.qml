import QtQuick
import QtQuick.Layouts
import QtCore
import AgIsoTc 1.0
import "components"

// Driving controls of the simulated tractor, floating over the 3D view: steering wheel,
// constant throttle and stop. Folds down to a single button.
GlassPanel {
    id: pad

    property bool keyboardActive: false
    property bool expanded: true

    Settings {
        category: "drivePad"
        property alias expanded: pad.expanded
    }

    implicitWidth: expanded ? 316 : collapsedButton.implicitWidth + 12
    implicitHeight: expanded ? content.implicitHeight + 24 : collapsedButton.implicitHeight + 12

    AppButton {

        focusPolicy: Qt.NoFocus
        id: collapsedButton
        visible: !pad.expanded
        anchors.centerIn: parent
        variant: "ghost"
        text: "Drive"
        iconName: "steering"
        onClicked: pad.expanded = true
    }

    ColumnLayout {
        id: content
        visible: pad.expanded
        anchors.fill: parent
        anchors.margins: 12
        spacing: 10

        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            Icon {
                name: "steering"
                size: 15
                color: Theme.accentText
            }
            Text {
                text: "Drive"
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSmall
                font.weight: Font.DemiBold
                color: Theme.text
            }
            StatusBadge {
                visible: pad.keyboardActive
                text: "Keyboard"
                tone: "accent"
            }
            Item { Layout.fillWidth: true }
            AppButton {
                focusPolicy: Qt.NoFocus
                variant: "ghost"
                size: "sm"
                iconName: "chevronDown"
                tip: "Fold the drive pad"
                onClicked: pad.expanded = false
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 14

            ColumnLayout {
                spacing: 4
                SteeringWheel {
                    Layout.alignment: Qt.AlignHCenter
                }
                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: Math.abs(bridge.steeringAngle) < 0.5 ? "Straight"
                          : Math.abs(bridge.steeringAngle).toFixed(0) + "° " + (bridge.steeringAngle < 0 ? "left" : "right")
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontCaption
                    font.weight: Font.DemiBold
                    color: Math.abs(bridge.steeringAngle) < 0.5 ? Theme.textMuted : Theme.accentText
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 6

                Text {
                    text: "SET SPEED"
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontCaption
                    font.weight: Font.DemiBold
                    font.letterSpacing: 0.5
                    color: Theme.textMuted
                }
                Row {
                    spacing: 4
                    Text {
                        id: speedText
                        text: bridge.throttleKph.toFixed(1)
                        font.family: Theme.fontFamily
                        font.pixelSize: 26
                        font.weight: Font.DemiBold
                        color: bridge.throttleKph > 0 ? Theme.text : Theme.textMuted
                    }
                    Text {
                        anchors.baseline: speedText.baseline
                        text: "km/h"
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontSmall
                        color: Theme.textMuted
                    }
                }

                AppSlider {

                    focusPolicy: Qt.NoFocus
                    Layout.fillWidth: true
                    Layout.leftMargin: -8
                    Layout.rightMargin: -8
                    from: 0
                    to: 50
                    stepSize: 0.5
                    value: bridge.throttleKph
                    onMoved: bridge.setThrottleKph(value)
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    AppButton {
                        focusPolicy: Qt.NoFocus
                        size: "sm"
                        iconName: "minus"
                        tip: "1 km/h slower (S)"
                        onClicked: bridge.adjustThrottle(-1)
                    }
                    AppButton {
                        focusPolicy: Qt.NoFocus
                        size: "sm"
                        iconName: "plus"
                        tip: "1 km/h faster (W)"
                        onClicked: bridge.adjustThrottle(1)
                    }
                    AppButton {
                        focusPolicy: Qt.NoFocus
                        Layout.fillWidth: true
                        size: "sm"
                        variant: "danger"
                        text: "Stop"
                        iconName: "stop"
                        tip: "Stop the tractor (Space)"
                        onClicked: bridge.stopTractor()
                    }
                }

                AppButton {

                    focusPolicy: Qt.NoFocus
                    Layout.fillWidth: true
                    size: "sm"
                    variant: "ghost"
                    text: "Centre wheel"
                    enabled: Math.abs(bridge.steeringAngle) >= 0.5
                    onClicked: bridge.setSteeringAngle(0)
                }
            }
        }
    }
}
