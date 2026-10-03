import QtQuick
import AgIsoTc 1.0

// Translucent floating surface for the heads-up displays over the 3D view and the map. It
// takes the mouse presses and wheel turns on it, so they do not reach the view underneath.
Rectangle {
    color: Theme.hudBg
    border.color: Theme.hudBorder
    radius: Theme.radiusLg

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.AllButtons
        onWheel: function(wheel) { wheel.accepted = true }
    }
}
