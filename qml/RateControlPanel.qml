import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import AgIsoTc 1.0
import "components"

// TC-GEO: the selected client's position-based control channels (ISO 11783-10 F.3.4). Per rate
// of a channel: where its setpoint comes from while a task is active (off, a fixed rate, or a
// layer of the task's prescription: variable rate), what the TC commands and what the client
// reports, and for a multi-rate device the rate of each sub-boom or section at its own position.
ScrollPage {
    id: root
    padding: 14
    spacing: 14

    readonly property var layers: bridge.prescription.present ? bridge.prescription.layers : []

    // A raw DDOP value as the client's value presentation shows it.
    function shown(raw, group) {
        if (raw === undefined || raw === null)
            return "—"
        const value = (Number(raw) + group.displayOffset) * group.displayScale
        const decimals = group.displayScale >= 1 ? 0 : (group.displayScale >= 0.01 ? 1 : 2)
        return value.toLocaleString(Qt.locale(), 'f', decimals) + (group.unit !== "" ? " " + group.unit : "")
    }

    function liveOf(target) {
        const value = bridge.rateLive["t" + target]
        return value !== undefined ? value : ({})
    }

    function sourceNames() {
        const names = ["Off", "Fixed rate"]
        for (let i = 0; i < root.layers.length; ++i)
            names.push("Map: " + root.layers[i].name)
        return names
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 12
        StatusBadge {
            Layout.maximumWidth: 460
            text: bridge.rateControlStatus
            tone: bridge.rateControlStatus.startsWith("Controlling") ? "success"
                  : (bridge.rateControlStatus.startsWith("Ready") ? "info" : "neutral")
            pulse: bridge.rateControlStatus.startsWith("Controlling")
        }
        Item { Layout.fillWidth: true }
        Text {
            Layout.maximumWidth: 420
            text: bridge.prescription.present ? "Prescription: " + bridge.prescription.name + " · " + bridge.prescription.description
                                              : "The selected task has no prescription (Fields page)"
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSmall
            color: Theme.textMuted
            elide: Text.ElideRight
        }
    }

    EmptyState {
        visible: bridge.rateChannels.length === 0
        Layout.alignment: Qt.AlignHCenter
        Layout.preferredWidth: Math.min(root.width - 32, 460)
        iconName: "layers"
        title: "No rate setpoints"
        text: "Select a client whose DDOP has settable rates. Each device element with a prescription control state is a control channel."
    }

    Repeater {
        model: bridge.rateChannels

        delegate: Rectangle {
            id: channelCard
            required property var modelData
            readonly property var live: bridge.rateLive["c" + modelData.index] || ({})

            Layout.fillWidth: true
            implicitHeight: channelColumn.implicitHeight + 24
            radius: Theme.radiusMd
            color: Theme.surfaceAlt
            border.color: live.engaged ? Theme.alpha(Theme.accent, 0.6) : Theme.border

            ColumnLayout {
                id: channelColumn
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 12
                spacing: 10

                // the channel: its element and its prescription control state
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10
                    Icon {
                        name: "layers"
                        size: 16
                        color: channelCard.live.engaged ? Theme.accentText : Theme.textMuted
                    }
                    Text {
                        text: (channelCard.modelData.name !== "" ? channelCard.modelData.name : "Element " + channelCard.modelData.element)
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontBody
                        font.weight: Font.DemiBold
                        color: Theme.text
                    }
                    Text {
                        text: "element " + channelCard.modelData.element
                        font.family: Theme.monoFamily
                        font.pixelSize: Theme.fontCaption
                        color: Theme.textMuted
                    }
                    Item { Layout.fillWidth: true }
                    StatusBadge {
                        text: channelCard.modelData.hasState ? "Prescription control: " + channelCard.live.state
                                                             : "No prescription control state"
                        tone: channelCard.live.state === "automatic" ? "success"
                              : (channelCard.live.state === "error" ? "danger" : "neutral")
                        dot: channelCard.modelData.hasState
                    }
                    Text {
                        visible: channelCard.live.latencyMs !== undefined && channelCard.live.latencyMs !== null
                        text: "latency " + channelCard.live.latencyMs + " ms"
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontCaption
                        color: Theme.textMuted
                        AppToolTip {
                            text: "Physical setpoint time latency: the TC looks the map up this far ahead of the element (ISO 11783-10 F.3.4.6)."
                            visible: latencyMouse.containsMouse
                        }
                        MouseArea {
                            id: latencyMouse
                            anchors.fill: parent
                            hoverEnabled: true
                        }
                    }
                }

                // one row per rate of the channel
                Repeater {
                    model: channelCard.modelData.groups

                    delegate: ColumnLayout {
                        id: groupItem
                        required property var modelData
                        readonly property var group: modelData
                        readonly property var topLive: group.top ? root.liveOf(group.top.target) : ({})

                        Layout.fillWidth: true
                        spacing: 8

                        Divider { Layout.fillWidth: true }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 12

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 0
                                Text {
                                    text: groupItem.group.name + (groupItem.group.bin !== undefined && groupItem.group.bin !== null ? "  ·  bin " + groupItem.group.bin : "")
                                    font.family: Theme.fontFamily
                                    font.pixelSize: Theme.fontBody
                                    font.weight: Font.DemiBold
                                    color: Theme.text
                                }
                                Text {
                                    text: "DDI " + groupItem.group.ddi
                                          + (groupItem.group.subs.length > 0 ? "  ·  " + groupItem.group.subs.length + " sub-rates, each at its own position" : "")
                                          + (groupItem.group.automatic && groupItem.group.matchedLayer >= 0 ? "  ·  map layer matched by DDI" : "")
                                    font.family: Theme.fontFamily
                                    font.pixelSize: Theme.fontCaption
                                    color: Theme.textMuted
                                }
                            }

                            AppComboBox {
                                Layout.preferredWidth: 240
                                model: root.sourceNames()
                                currentIndex: groupItem.group.source
                                // a new list of layers resets the index: show the group's source again
                                onCountChanged: currentIndex = groupItem.group.source
                                onActivated: function(index) { bridge.setRateGroupSource(groupItem.group.index, index) }
                                AppToolTip {
                                    text: "Where the setpoint comes from while a task is active"
                                    visible: parent.hovered
                                }
                            }
                            AppSpinBox {
                                visible: groupItem.group.source === 1
                                Layout.preferredWidth: 150
                                from: 0
                                to: 2000000000
                                stepSize: 100
                                value: groupItem.group.fixed
                                onValueModified: bridge.setRateGroupFixed(groupItem.group.index, value)
                                AppToolTip {
                                    text: "Fixed rate in the DDOP's raw unit: " + root.shown(parent.value, groupItem.group)
                                    visible: parent.hovered
                                }
                            }

                            // the channel-level setpoint: commanded and actual
                            ColumnLayout {
                                visible: groupItem.group.top !== undefined && groupItem.group.top !== null
                                Layout.preferredWidth: 170
                                spacing: 0
                                Text {
                                    Layout.alignment: Qt.AlignRight
                                    text: root.shown(groupItem.topLive.commanded, groupItem.group)
                                    font.family: Theme.monoFamily
                                    font.pixelSize: Theme.fontTitle
                                    font.weight: Font.DemiBold
                                    color: groupItem.topLive.commanded !== undefined && groupItem.topLive.commanded !== null ? Theme.accentText : Theme.textMuted
                                }
                                Text {
                                    Layout.alignment: Qt.AlignRight
                                    text: "actual " + root.shown(groupItem.topLive.actual, groupItem.group)
                                          + (groupItem.topLive.source ? "  ·  " + groupItem.topLive.source : "")
                                    font.family: Theme.fontFamily
                                    font.pixelSize: Theme.fontCaption
                                    color: Theme.textMuted
                                }
                            }
                        }

                        // multi-rate: each sub-boom or section with its own rate
                        Flow {
                            visible: groupItem.group.subs.length > 0
                            Layout.fillWidth: true
                            spacing: 6

                            Repeater {
                                model: groupItem.group.subs
                                delegate: Rectangle {
                                    id: subTile
                                    required property var modelData
                                    readonly property var live: root.liveOf(modelData.target)
                                    readonly property bool commanded: live.commanded !== undefined && live.commanded !== null

                                    width: 128
                                    height: 52
                                    radius: Theme.radiusSm
                                    color: commanded ? Theme.alpha(Theme.accent, Theme.dark ? 0.12 : 0.08) : Theme.surface
                                    border.color: commanded ? Theme.alpha(Theme.accent, 0.5) : Theme.border

                                    Column {
                                        anchors.fill: parent
                                        anchors.margins: 6
                                        spacing: 1
                                        Text {
                                            width: parent.width
                                            text: "#" + subTile.modelData.element + "  " + subTile.modelData.name
                                            font.family: Theme.fontFamily
                                            font.pixelSize: Theme.fontCaption
                                            color: Theme.textMuted
                                            elide: Text.ElideRight
                                        }
                                        Text {
                                            text: root.shown(subTile.live.commanded, groupItem.group)
                                            font.family: Theme.monoFamily
                                            font.pixelSize: Theme.fontSmall
                                            font.weight: Font.DemiBold
                                            color: subTile.commanded ? Theme.accentText : Theme.textMuted
                                        }
                                        Text {
                                            width: parent.width
                                            text: "act " + root.shown(subTile.live.actual, groupItem.group)
                                            font.family: Theme.fontFamily
                                            font.pixelSize: Theme.fontCaption
                                            color: Theme.textMuted
                                            elide: Text.ElideRight
                                        }
                                    }
                                    AppToolTip {
                                        text: "Map: " + (subTile.live.source ? subTile.live.source : "—")
                                              + ", wanted " + root.shown(subTile.live.wanted, groupItem.group)
                                        visible: tileMouse.containsMouse
                                    }
                                    MouseArea {
                                        id: tileMouse
                                        anchors.fill: parent
                                        hoverEnabled: true
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    Text {
        Layout.fillWidth: true
        visible: bridge.rateChannels.length > 0
        text: "While a task is active the TC sets each channel's prescription control state to automatic, then sends its rates: a fixed rate to the channel (the device passes it on to its sub-booms), or the map's rate for each sub-boom or section at the point it reaches after its setpoint latency. A rate is sent when it changes, and every 2 s."
        wrapMode: Text.Wrap
        font.family: Theme.fontFamily
        font.pixelSize: Theme.fontCaption
        color: Theme.textMuted
    }
}
