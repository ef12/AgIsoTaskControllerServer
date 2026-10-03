import QtQuick
import AgIsoTc 1.0

// The Task Controller data in a window of its own.
Window {
    id: root
    visible: false
    width: 1080
    height: 680
    minimumWidth: 760
    minimumHeight: 420
    title: "Task Controller data"
    color: Theme.bg

    ProcessDataPanel {
        anchors.fill: parent
        anchors.margins: 10
        detached: true
    }
}
