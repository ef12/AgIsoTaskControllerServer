import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: root
    visible: true
    width: 1440
    height: 900
    minimumWidth: 1100
    minimumHeight: 700
    title: "AgIso Task Controller Server"
    color: "#1b1e24"

    // Drain the core event queue into the models.
    Timer {
        interval: 150
        running: true
        repeat: true
        onTriggered: bridge.poll()
    }

    Timer {
        id: identifyTimer
        interval: 3000
    }

    Connections {
        target: bridge
        function onIdentifyBanner(tcNumber) {
            identifyLabel.text = "TC " + tcNumber + " — identify requested";
            identifyTimer.restart();
        }
    }

    Rectangle {
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.topMargin: 56
        width: identifyLabel.implicitWidth + 32
        height: 40
        radius: 8
        color: "#2f4a63"
        border.color: "#5aa2e0"
        visible: identifyTimer.running
        z: 10
        Label {
            id: identifyLabel
            anchors.centerIn: parent
            color: "white"
            font.bold: true
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 8

        TopBar {
            Layout.fillWidth: true
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 8

            ColumnLayout {
                Layout.preferredWidth: 300
                Layout.fillHeight: true
                spacing: 8
                ClientsPanel {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 340
                }
                CommandPanel {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 8
                SectionView3D {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                }
                ProcessDataPanel {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 280
                }
            }

            ColumnLayout {
                Layout.preferredWidth: 400
                Layout.fillHeight: true
                spacing: 8
                DdopPanel {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                }
                LogPanel {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 280
                }
            }
        }

        Label {
            text: bridge.statusText
            color: "#9fb0c3"
            font.pixelSize: 12
        }
    }
}
