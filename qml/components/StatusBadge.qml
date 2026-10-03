import QtQuick
import AgIsoTc 1.0

// A small rounded status label with a coloured dot, e.g. "Running" or "Timeout".
//   tone:  "neutral", "accent", "success", "warning", "danger" or "info"
//   pulse: the dot pulses, for live states
Rectangle {
    id: root

    property string text
    property string tone: "neutral"
    property bool pulse: false
    property bool dot: true

    readonly property color toneColor: Theme.tone(tone)

    implicitHeight: 22
    implicitWidth: row.implicitWidth + 16
    radius: height / 2
    color: Theme.alpha(toneColor, Theme.dark ? 0.14 : 0.1)
    border.color: Theme.alpha(toneColor, 0.3)

    Row {
        id: row
        anchors.centerIn: parent
        spacing: 6

        PulseDot {
            visible: root.dot
            anchors.verticalCenter: parent.verticalCenter
            size: 7
            color: root.toneColor
            pulse: root.pulse
        }

        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: root.text
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontCaption
            font.weight: Font.DemiBold
            color: root.tone === "neutral" ? Theme.textSecondary : root.toneColor
        }
    }
}
