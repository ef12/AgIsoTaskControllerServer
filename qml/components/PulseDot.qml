import QtQuick
import AgIsoTc 1.0

// A status dot; with `pulse` it sends out a fading ring, for live states.
Item {
    id: root

    property color color: Theme.textMuted
    property bool pulse: false
    property real size: 8

    implicitWidth: size
    implicitHeight: size

    Rectangle {
        id: ring
        anchors.centerIn: parent
        width: root.size
        height: root.size
        radius: width / 2
        color: root.color
        opacity: 0

        ParallelAnimation {
            running: root.pulse && root.visible
            loops: Animation.Infinite
            onStopped: ring.opacity = 0
            NumberAnimation { target: ring; property: "scale"; from: 1; to: 2.8; duration: 1500; easing.type: Easing.OutCubic }
            NumberAnimation { target: ring; property: "opacity"; from: 0.6; to: 0; duration: 1500; easing.type: Easing.OutCubic }
        }
    }

    Rectangle {
        anchors.fill: parent
        radius: width / 2
        color: root.color
    }
}
