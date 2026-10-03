import QtQuick
import QtQuick.Layouts
import AgIsoTc 1.0
import "../components"

// The CAN interface, what the TC reports it supports, the start/stop of the server, and the
// devices heard on the bus. The settings are locked while the server runs.
ScrollPage {
    id: page

    property var config

    readonly property int connectedPeers: {
        let count = 0
        for (let i = 0; i < bridge.busPeers.length; ++i)
            count += bridge.busPeers[i].connected ? 1 : 0
        return count
    }

    PageHeader {
        Layout.fillWidth: true
        title: "Connection"
        subtitle: "The CAN interface and what this task controller offers its clients"
    }

    // --- server state and the main action ----------------------------------------------------
    Card {
        Layout.fillWidth: true
        color: bridge.running ? Theme.alpha(Theme.accent, Theme.dark ? 0.07 : 0.05) : Theme.surfaceAlt
        border.color: bridge.running ? Theme.alpha(Theme.accent, 0.3) : Theme.border

        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            Rectangle {
                implicitWidth: 40
                implicitHeight: 40
                radius: 12
                color: bridge.running ? Theme.alpha(Theme.accent, 0.16) : Theme.surfacePressed
                Icon {
                    anchors.centerIn: parent
                    name: "power"
                    size: 20
                    color: bridge.running ? Theme.accentText : Theme.textMuted
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2
                Text {
                    text: bridge.running ? "Server running" : "Server stopped"
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontTitle
                    font.weight: Font.DemiBold
                    color: Theme.text
                }
                Text {
                    Layout.fillWidth: true
                    text: page.config ? page.config.summary : ""
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSmall
                    color: Theme.textMuted
                    elide: Text.ElideRight
                }
            }

            StatusBadge {
                text: bridge.running ? "Online" : "Offline"
                tone: bridge.running ? "success" : "neutral"
                pulse: bridge.running
            }
        }

        AppButton {
            Layout.fillWidth: true
            size: "lg"
            text: bridge.running ? "Stop server" : "Start server"
            iconName: "power"
            variant: bridge.running ? "dangerSoft" : "primary"
            onClicked: {
                if (bridge.running)
                    bridge.stopServer()
                else
                    page.config.start()
            }
        }
    }

    // --- CAN interface ---------------------------------------------------------------------
    Card {
        Layout.fillWidth: true
        title: "CAN interface"
        subtitle: bridge.running ? "Stop the server to change the interface." : ""
        iconName: "network"

        FormField {
            Layout.fillWidth: true
            label: "Driver"
            hint: page.config ? page.config.current.hint : ""
            AppComboBox {
                Layout.fillWidth: true
                enabled: !bridge.running
                model: page.config ? page.config.drivers : []
                textRole: "label"
                currentIndex: page.config ? page.config.driverIndex : 0
                onActivated: function(index) { page.config.selectDriver(page.config.drivers[index].key) }
            }
        }

        FormField {
            Layout.fillWidth: true
            visible: page.config ? page.config.current.usesChannel : true
            label: page.config ? page.config.current.channelLabel : "Channel"
            AppTextField {
                Layout.fillWidth: true
                enabled: !bridge.running
                text: page.config ? page.config.channel : ""
                onTextEdited: page.config.channel = text
            }
        }
    }

    // --- TC capabilities -------------------------------------------------------------------
    Card {
        Layout.fillWidth: true
        title: "Task controller"
        subtitle: "Offer at least what the client reports (see the event log), e.g. 64 sections."
        iconName: "cpu"

        GridLayout {
            Layout.fillWidth: true
            columns: 2
            columnSpacing: 12
            rowSpacing: 12

            FormField {
                Layout.fillWidth: true
                label: "TC number"
                AppSpinBox {
                    Layout.fillWidth: true
                    enabled: !bridge.running
                    from: 1
                    to: 32
                    value: page.config ? page.config.tcNumber : 1
                    onValueModified: page.config.tcNumber = value
                }
            }
            FormField {
                Layout.fillWidth: true
                label: "Booms"
                AppSpinBox {
                    Layout.fillWidth: true
                    enabled: !bridge.running
                    from: 1
                    to: 255
                    value: page.config ? page.config.booms : 4
                    onValueModified: page.config.booms = value
                }
            }
            FormField {
                Layout.fillWidth: true
                label: "Sections"
                AppSpinBox {
                    Layout.fillWidth: true
                    enabled: !bridge.running
                    from: 1
                    to: 255
                    value: page.config ? page.config.sections : 64
                    onValueModified: page.config.sections = value
                }
            }
            FormField {
                Layout.fillWidth: true
                label: "Control channels"
                AppSpinBox {
                    Layout.fillWidth: true
                    enabled: !bridge.running
                    from: 0
                    to: 255
                    value: page.config ? page.config.channels : 16
                    onValueModified: page.config.channels = value
                }
            }
        }
    }

    // --- devices heard on the bus ----------------------------------------------------------
    Card {
        Layout.fillWidth: true
        title: "Heard on the bus"
        subtitle: bridge.busPeers.length === 0
                  ? "Address claims appear here, connected to this TC or not."
                  : bridge.busPeers.length + (bridge.busPeers.length === 1 ? " device, " : " devices, ")
                    + page.connectedPeers + " connected to this TC"
        iconName: "radio"
        iconColor: Theme.info

        Repeater {
            model: bridge.busPeers

            delegate: Rectangle {
                Layout.fillWidth: true
                implicitHeight: 44
                radius: Theme.radiusMd
                color: peerMouse.containsMouse ? Theme.surfaceHover : Theme.surface
                border.color: Theme.border

                MouseArea {
                    id: peerMouse
                    anchors.fill: parent
                    hoverEnabled: true
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 10
                    anchors.rightMargin: 10
                    spacing: 10

                    Rectangle {
                        implicitWidth: 34
                        implicitHeight: 26
                        radius: 6
                        color: Theme.surfacePressed
                        Text {
                            anchors.centerIn: parent
                            text: modelData.address
                            font.family: Theme.monoFamily
                            font.pixelSize: Theme.fontSmall
                            font.weight: Font.DemiBold
                            color: Theme.text
                        }
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 0
                        Text {
                            text: "Function " + modelData.functionCode + " / " + modelData.functionInstance
                            font.family: Theme.fontFamily
                            font.pixelSize: Theme.fontSmall
                            font.weight: Font.DemiBold
                            color: Theme.text
                        }
                        Text {
                            text: "Manufacturer " + modelData.manufacturerCode
                            font.family: Theme.fontFamily
                            font.pixelSize: Theme.fontCaption
                            color: Theme.textMuted
                        }
                    }
                    StatusBadge {
                        text: modelData.connected ? "TC client" : "Not connected"
                        tone: modelData.connected ? "success" : "warning"
                    }
                }
            }
        }
    }
}
