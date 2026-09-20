import QtQuick
import QtQuick.Controls

Window {
    id: root
    visible: false
    width: 1000
    height: 640
    minimumWidth: 760
    minimumHeight: 420
    title: "Task Controller data"
    color: "#1b1e24"

    ProcessDataPanel {
        anchors.fill: parent
        anchors.margins: 8
    }
}
