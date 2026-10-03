import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import AgIsoTc 1.0
import "../components"

// The TC clients and the device descriptor of the selected one, as master and detail: the
// client cards on top (click one to select it) and below them the selected client's DDOP, its
// object tree and its declared DDIs, with the pool actions, which act on that client.
Item {
    id: page

    signal showPageRequested(string key)

    readonly property bool clientSelected: bridge.selectedClient >= 0

    // "Device 1: Sprayer", "Element 3 (#2, parent 1): Boom", ... split for the inspector
    readonly property var objectKinds: ({
        "Device": { tag: "DVC", tone: "accent" },
        "Element": { tag: "DET", tone: "info" },
        "Process data": { tag: "DPD", tone: "violet" },
        "Property": { tag: "DPT", tone: "warning" },
        "Value presentation": { tag: "DVP", tone: "neutral" },
        "Object": { tag: "OBJ", tone: "neutral" }
    })

    function parseObjectRow(text) {
        const match = /^(Device|Element|Process data|Property|Value presentation|Object) (\d+)(?: \(([^)]*)\))?:\s*(.*)$/.exec(text)
        if (!match)
            return null
        return { kind: objectKinds[match[1]], id: match[2], detail: match[3] || "", rest: match[4] }
    }

    function toneColor(tone) {
        return tone === "violet" ? Theme.violet : Theme.tone(tone)
    }

    function formatBytes(bytes) {
        if (bytes >= 1024)
            return (bytes / 1024).toFixed(1) + " kB"
        return bytes + " B"
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.s4
        spacing: Theme.s3

        PageHeader {
            Layout.fillWidth: true
            title: "Implements"
            subtitle: clientList.count === 0 ? "TC clients and their device descriptors"
                                             : clientList.count + (clientList.count === 1 ? " TC client" : " TC clients")
                                               + " · select one to inspect it"
        }

        // --- the clients -------------------------------------------------------------------
        Rectangle {
            visible: clientList.count === 0
            Layout.fillWidth: true
            implicitHeight: noClients.implicitHeight + 40
            radius: Theme.radiusLg
            color: Theme.surfaceAlt
            border.color: Theme.border

            EmptyState {
                id: noClients
                anchors.centerIn: parent
                width: parent.width - 32
                iconName: "box"
                title: "No TC client yet"
                text: bridge.running ? "Waiting for an implement to connect. It shows up here once it sends its working set and starts talking to this TC."
                                     : "Start the server, then connect an implement on the same bus."
                AppButton {
                    visible: !bridge.running
                    text: "Go to connection"
                    iconName: "network"
                    size: "sm"
                    onClicked: page.showPageRequested("connection")
                }
            }
        }

        ListView {
            id: clientList
            visible: count > 0
            Layout.fillWidth: true
            Layout.preferredHeight: Math.min(3, count) * 82 - 8
            model: clientModel
            clip: true
            spacing: 8
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: AppScrollBar { }

            delegate: Rectangle {
                id: clientCard

                readonly property bool selected: clientAddress === bridge.selectedClient

                width: clientList.width
                height: 74
                radius: 10
                color: selected ? Theme.alpha(Theme.accent, Theme.dark ? 0.09 : 0.06)
                                : (cardMouse.containsMouse ? Theme.surfaceHover : Theme.surfaceAlt)
                border.color: selected ? Theme.alpha(Theme.accent, 0.55) : Theme.border
                Behavior on color { ColorAnimation { duration: Theme.fast } }

                MouseArea {
                    id: cardMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: bridge.selectClient(clientAddress)
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 12

                    Rectangle {
                        implicitWidth: 44
                        implicitHeight: 44
                        radius: 11
                        color: clientCard.selected ? Theme.alpha(Theme.accent, 0.18) : Theme.surfacePressed
                        Column {
                            anchors.centerIn: parent
                            spacing: -2
                            Text {
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: "SA"
                                font.family: Theme.fontFamily
                                font.pixelSize: 9
                                font.weight: Font.DemiBold
                                color: Theme.textMuted
                            }
                            Text {
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: clientAddress
                                font.family: Theme.monoFamily
                                font.pixelSize: Theme.fontBody
                                font.weight: Font.Bold
                                color: clientCard.selected ? Theme.accentText : Theme.text
                            }
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 4

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 6
                            Text {
                                Layout.fillWidth: true
                                text: "Function " + clientFunction
                                font.family: Theme.fontFamily
                                font.pixelSize: Theme.fontBody
                                font.weight: Font.DemiBold
                                color: Theme.text
                                elide: Text.ElideRight
                            }
                            StatusBadge {
                                visible: timedOut
                                text: "Timeout"
                                tone: "danger"
                            }
                            StatusBadge {
                                visible: !timedOut
                                text: ddopActive ? "Pool active" : (ddopSize > 0 ? "Pool stored" : "No pool")
                                tone: ddopActive ? "success" : (ddopSize > 0 ? "info" : "neutral")
                            }
                        }
                        Text {
                            Layout.fillWidth: true
                            text: "TC v" + reportedVersion + "  ·  Mfr " + manufacturerCode + "  ·  DDOP "
                                  + page.formatBytes(ddopSize) + (lastSeen !== "" ? "  ·  " + lastSeen : "")
                            font.family: Theme.fontFamily
                            font.pixelSize: Theme.fontCaption
                            color: Theme.textMuted
                            elide: Text.ElideRight
                        }
                        Text {
                            Layout.fillWidth: true
                            text: "NAME " + clientName
                            font.family: Theme.monoFamily
                            font.pixelSize: Theme.fontCaption
                            color: Theme.textMuted
                            elide: Text.ElideRight
                        }
                    }
                }
            }
        }

        Divider {
            Layout.fillWidth: true
            Layout.topMargin: 4
        }

        // --- the selected client's DDOP ----------------------------------------------------
        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 1
                Text {
                    text: "Device descriptor"
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontTitle
                    font.weight: Font.DemiBold
                    color: Theme.text
                }
                Text {
                    Layout.fillWidth: true
                    text: page.clientSelected ? "Object pool of client SA " + bridge.selectedClient
                                              : "Select a client to see its DDOP"
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSmall
                    color: Theme.textMuted
                    elide: Text.ElideRight
                }
            }
            AppButton {
                variant: "ghost"
                iconName: "upload"
                enabled: page.clientSelected
                tip: "Load a DDOP binary (.bin, .iop) for the selected client"
                onClicked: poolDialog.open()
            }
            AppButton {
                variant: "ghost"
                iconName: "trash"
                enabled: page.clientSelected
                tip: "Clear the selected client's pool"
                onClicked: bridge.clearPool()
            }
        }

        Item {
            visible: !page.clientSelected
            Layout.fillWidth: true
            Layout.fillHeight: true
            EmptyState {
                anchors.centerIn: parent
                width: parent.width - 32
                iconName: "layers"
                title: "Nothing selected"
                text: "The device, its elements, process data and properties appear here."
            }
        }

        Tabs {
            id: tabs
            visible: page.clientSelected
            Layout.fillWidth: true
            fill: true
            model: [{ "text": "Objects", "icon": "layers" },
                    { "text": "Declared DDIs", "icon": "list", "count": bridge.implementDdis.length }]
        }

        StackLayout {
            visible: page.clientSelected
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: tabs.currentIndex

            // object tree
            ListView {
                id: ddopList
                model: ddopModel
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                ScrollBar.vertical: AppScrollBar { }

                delegate: Item {
                    id: objectRow

                    readonly property var parsed: page.parseObjectRow(rowText)
                    readonly property color kindColor: parsed ? page.toneColor(parsed.kind.tone) : Theme.textMuted

                    width: ddopList.width
                    height: Math.max(28, rowContent.implicitHeight + 8)

                    Rectangle {
                        anchors.fill: parent
                        radius: Theme.radiusSm
                        color: rowMouse.containsMouse && objectRow.parsed ? Theme.surfaceHover : Theme.alpha(Theme.surfaceHover, 0)
                    }
                    MouseArea {
                        id: rowMouse
                        anchors.fill: parent
                        hoverEnabled: true
                    }

                    // indentation guides
                    Repeater {
                        model: objectRow.parsed ? Math.max(0, indent) : 0
                        delegate: Rectangle {
                            x: 13 + index * 18
                            width: 1
                            height: objectRow.height
                            color: Theme.divider
                        }
                    }

                    RowLayout {
                        id: rowContent
                        x: objectRow.parsed ? 6 + indent * 18 : 6
                        width: parent.width - x - 6
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 8

                        Rectangle {
                            visible: !!objectRow.parsed
                            implicitWidth: 34
                            implicitHeight: 18
                            radius: 4
                            color: Theme.alpha(objectRow.kindColor, 0.15)
                            Text {
                                anchors.centerIn: parent
                                text: objectRow.parsed ? objectRow.parsed.kind.tag : ""
                                font.family: Theme.monoFamily
                                font.pixelSize: 10
                                font.weight: Font.Bold
                                color: objectRow.kindColor
                            }
                        }
                        Text {
                            visible: !!objectRow.parsed
                            text: objectRow.parsed ? objectRow.parsed.id : ""
                            font.family: Theme.monoFamily
                            font.pixelSize: Theme.fontSmall
                            color: Theme.textMuted
                        }
                        Text {
                            Layout.fillWidth: true
                            text: objectRow.parsed ? objectRow.parsed.rest : rowText
                            font.family: Theme.fontFamily
                            font.pixelSize: objectRow.parsed ? Theme.fontBody : Theme.fontSmall
                            font.weight: objectRow.parsed && objectRow.parsed.kind.tag === "DVC" ? Font.DemiBold : Font.Normal
                            color: objectRow.parsed ? Theme.text : Theme.textMuted
                            wrapMode: Text.Wrap
                        }
                        Text {
                            visible: !!objectRow.parsed && objectRow.parsed.detail !== ""
                            text: objectRow.parsed ? objectRow.parsed.detail : ""
                            font.family: Theme.monoFamily
                            font.pixelSize: Theme.fontCaption
                            color: Theme.textMuted
                        }
                    }
                }
            }

            // declared DDIs
            ColumnLayout {
                spacing: 8

                RowLayout {
                    Layout.fillWidth: true
                    Layout.topMargin: 4
                    spacing: 8
                    AppSwitch {
                        text: "Auto sync"
                        checked: bridge.autoDdiSync
                        onToggled: bridge.setAutoDdiSync(checked)
                        AppToolTip {
                            text: "Request the declared DDIs one after the other, at this interval"
                            visible: parent.hovered
                        }
                    }
                    AppSpinBox {
                        Layout.preferredWidth: 120
                        from: 250
                        to: 60000
                        stepSize: 250
                        value: bridge.ddiSyncIntervalMs
                        textFromValue: function(value, locale) { return value + " ms" }
                        valueFromText: function(text, locale) { return parseInt(text) }
                        validator: RegularExpressionValidator { regularExpression: /\d{1,5}( ?ms)?/ }
                        onValueModified: bridge.setDdiSyncIntervalMs(value)
                    }
                    Item { Layout.fillWidth: true }
                    AppButton {
                        size: "sm"
                        text: "Request all"
                        iconName: "refresh"
                        onClicked: bridge.requestImplementDdis()
                    }
                }

                ListView {
                    id: ddiList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    model: bridge.implementDdis
                    clip: true
                    spacing: 2
                    boundsBehavior: Flickable.StopAtBounds
                    ScrollBar.vertical: AppScrollBar { }

                    delegate: Rectangle {
                        width: ddiList.width
                        height: 48
                        radius: Theme.radiusSm
                        color: ddiMouse.containsMouse ? Theme.surfaceHover : (index % 2 ? Theme.alpha(Theme.surfaceAlt, 0.6) : Theme.alpha(Theme.surfaceAlt, 0))

                        MouseArea {
                            id: ddiMouse
                            anchors.fill: parent
                            hoverEnabled: true
                        }

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 8
                            anchors.rightMargin: 8
                            spacing: 10

                            Rectangle {
                                implicitWidth: 52
                                implicitHeight: 22
                                radius: 5
                                color: Theme.surfacePressed
                                Text {
                                    anchors.centerIn: parent
                                    text: modelData.ddi
                                    font.family: Theme.monoFamily
                                    font.pixelSize: Theme.fontSmall
                                    font.weight: Font.DemiBold
                                    color: Theme.textSecondary
                                }
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 1
                                Text {
                                    Layout.fillWidth: true
                                    text: modelData.name !== "" ? modelData.name : "DDI " + modelData.ddi
                                    font.family: Theme.fontFamily
                                    font.pixelSize: Theme.fontSmall
                                    font.weight: Font.DemiBold
                                    color: Theme.text
                                    elide: Text.ElideRight
                                }
                                Text {
                                    Layout.fillWidth: true
                                    text: "element " + modelData.element + "  ·  " + modelData.triggers
                                          + (modelData.settable ? "  ·  settable" : "")
                                    font.family: Theme.fontFamily
                                    font.pixelSize: Theme.fontCaption
                                    color: Theme.textMuted
                                    elide: Text.ElideRight
                                }
                            }

                            ColumnLayout {
                                spacing: 1
                                Text {
                                    Layout.alignment: Qt.AlignRight
                                    text: modelData.hasValue ? modelData.value : "—"
                                    font.family: Theme.monoFamily
                                    font.pixelSize: Theme.fontBody
                                    font.weight: Font.DemiBold
                                    color: modelData.hasValue ? Theme.accentText : Theme.textMuted
                                }
                                Text {
                                    Layout.alignment: Qt.AlignRight
                                    text: modelData.hasValue ? modelData.updated : "waiting"
                                    font.family: Theme.fontFamily
                                    font.pixelSize: Theme.fontCaption
                                    color: Theme.textMuted
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    FileDialog {
        id: poolDialog
        title: "Open DDOP binary"
        nameFilters: ["DDOP files (*.bin *.iop *.ddop)", "All files (*)"]
        onAccepted: bridge.loadPoolFile(selectedFile)
    }
}
