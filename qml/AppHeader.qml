import QtQuick
import QtQuick.Layouts
import AgIsoTc 1.0
import "components"

// The top bar: the application, the server and task status (click to open their page), the
// windows, and the two main actions: start or stop the task and the server.
Item {
    id: root

    property var config

    signal connectionClicked()
    signal taskClicked()
    signal openFieldMapRequested()
    signal openTaskDataRequested()

    implicitHeight: 60

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 18
        anchors.rightMargin: 12
        spacing: 12

        Image {
            Layout.preferredWidth: 30
            Layout.preferredHeight: 30
            source: "qrc:/icons/logo.ico"
            sourceSize: Qt.size(64, 64)
            smooth: true
            mipmap: true
        }

        ColumnLayout {
            spacing: 0
            Text {
                text: "AgIso Task Controller"
                font.family: Theme.fontFamily
                font.pixelSize: 15
                font.weight: Font.DemiBold
                color: Theme.text
            }
            Text {
                text: "ISO 11783-10 server"
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontCaption
                color: Theme.textMuted
            }
        }

        Item { Layout.preferredWidth: 8 }

        HeaderChip {
            title: bridge.running ? "Server running" : "Server stopped"
            detail: root.config ? root.config.summary : ""
            tone: bridge.running ? "success" : "neutral"
            pulse: bridge.running
            tip: "CAN interface and TC capabilities"
            onClicked: root.connectionClicked()
        }

        HeaderChip {
            title: bridge.taskActive ? "Recording" : (bridge.selectedTaskIndex >= 0 ? "Task ready" : "No task")
            detail: {
                if (bridge.taskActive)
                    return bridge.activeTaskName + " · " + bridge.activeFieldName
                if (bridge.selectedTaskIndex >= 0)
                    return bridge.taskNames[bridge.selectedTaskIndex] || ""
                return bridge.selectedFieldIndex >= 0 ? "Create a task for the field" : "Create a field first"
            }
            tone: bridge.taskActive ? "danger" : (bridge.selectedTaskIndex >= 0 ? "info" : "neutral")
            pulse: bridge.taskActive
            tip: "Fields and tasks"
            onClicked: root.taskClicked()
        }

        Item { Layout.fillWidth: true }

        AppButton {
            variant: "ghost"
            iconName: "map"
            text: "Field map"
            tip: "Open the 2D field map, where you can draw a field boundary"
            onClicked: root.openFieldMapRequested()
        }
        AppButton {
            variant: "ghost"
            iconName: "externalLink"
            text: "TC data"
            tip: "Open the Task Controller data in a window of its own"
            onClicked: root.openTaskDataRequested()
        }

        Rectangle {
            Layout.preferredWidth: 1
            Layout.preferredHeight: 24
            color: Theme.border
        }

        AppButton {
            text: bridge.taskActive ? "Stop task" : "Start task"
            iconName: bridge.taskActive ? "stop" : "play"
            variant: bridge.taskActive ? "dangerSoft" : "secondary"
            enabled: bridge.selectedTaskIndex >= 0
            onClicked: bridge.setTaskActive(!bridge.taskActive)
        }
        AppButton {
            Layout.preferredWidth: 136
            text: bridge.running ? "Stop server" : "Start server"
            iconName: "power"
            variant: bridge.running ? "dangerSoft" : "primary"
            onClicked: {
                if (bridge.running)
                    bridge.stopServer()
                else
                    root.config.start()
            }
        }
    }
}
