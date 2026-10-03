import QtQuick
import QtQuick.Layouts
import AgIsoTc 1.0
import "components"

// The bottom line: the last status message and the state of server, GPS, clients and bus.
Item {
    id: root

    property int clientCount: 0

    implicitHeight: 30

    component Indicator: Row {
        property string label
        property string value
        property string tone: "neutral"
        spacing: 6
        PulseDot {
            anchors.verticalCenter: parent.verticalCenter
            size: 6
            color: Theme.tone(parent.tone)
        }
        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: parent.label
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontCaption
            color: Theme.textMuted
        }
        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: parent.value
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontCaption
            font.weight: Font.DemiBold
            color: Theme.textSecondary
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 18
        anchors.rightMargin: 18
        spacing: 18

        Icon {
            name: "info"
            size: 13
            color: Theme.textMuted
        }
        Text {
            Layout.fillWidth: true
            Layout.leftMargin: -10
            text: bridge.statusText
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSmall
            color: Theme.textSecondary
            elide: Text.ElideRight
        }

        Indicator {
            label: "Server"
            value: bridge.running ? "running" : "stopped"
            tone: bridge.running ? "success" : "neutral"
        }
        Indicator {
            label: "GPS"
            value: bridge.gpsValid ? "fix" : (bridge.gpsRunning ? "searching" : "off")
            tone: bridge.gpsValid ? "success" : (bridge.gpsRunning ? "warning" : "neutral")
        }
        Indicator {
            label: "Clients"
            value: root.clientCount
            tone: root.clientCount > 0 ? "info" : "neutral"
        }
        Indicator {
            label: "Bus"
            value: bridge.busPeers.length + (bridge.busPeers.length === 1 ? " device" : " devices")
            tone: bridge.busPeers.length > 0 ? "info" : "neutral"
        }
    }
}
