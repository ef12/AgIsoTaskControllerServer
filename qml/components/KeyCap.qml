import QtQuick
import AgIsoTc 1.0

// A keyboard key, for shortcut hints.
Rectangle {
    id: root

    property string text

    implicitWidth: Math.max(20, label.implicitWidth + 10)
    implicitHeight: 20
    radius: 4
    color: Theme.surfaceAlt
    border.color: Theme.borderStrong

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 1
        height: 2
        radius: 2
        color: Theme.border
    }

    Text {
        id: label
        anchors.centerIn: parent
        anchors.verticalCenterOffset: -1
        text: root.text
        font.family: Theme.monoFamily
        font.pixelSize: Theme.fontCaption
        font.weight: Font.DemiBold
        color: Theme.textSecondary
    }
}
