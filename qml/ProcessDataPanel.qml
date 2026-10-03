import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import AgIsoTc 1.0
import "components"

// The Task Controller data: TC-Basic values, TC-SC (section control, coverage, rate control),
// raw process data, DDI traffic and the event log. Docked under the 3D view, where it folds
// down to its tab row, or shown in a window of its own (detached).
Rectangle {
    id: root

    property bool detached: false
    property bool collapsed: false

    signal collapseToggled()
    signal popOutRequested()

    radius: Theme.radiusLg
    color: Theme.surface
    border.color: Theme.border
    clip: true

    // log tags and their colours: "[gps] Source started." -> gps
    readonly property var tagTones: ({
        "bus": "info", "gps": "violet", "ddop": "accent", "ddi": "warning",
        "field": "success", "task": "success", "tc": "info", "tc-sc": "accent", "rate": "warning"
    })

    function toneColor(tone) {
        return tone === "violet" ? Theme.violet : Theme.tone(tone)
    }

    function formatDuration(seconds) {
        const total = Math.floor(seconds)
        const h = Math.floor(total / 3600)
        const m = Math.floor((total % 3600) / 60)
        const s = total % 60
        const pad = function(n) { return n < 10 ? "0" + n : "" + n }
        return (h > 0 ? h + ":" + pad(m) : m) + ":" + pad(s)
    }

    component ColumnTitle: Text {
        font.family: Theme.fontFamily
        font.pixelSize: Theme.fontCaption
        font.weight: Font.DemiBold
        font.letterSpacing: 0.4
        font.capitalization: Font.AllUppercase
        color: Theme.textMuted
        elide: Text.ElideRight
    }

    component Cell: Text {
        font.family: Theme.monoFamily
        font.pixelSize: Theme.fontSmall
        color: Theme.text
        elide: Text.ElideRight
        verticalAlignment: Text.AlignVCenter
    }

    // A figure on a tile, for the TC-SC totals.
    component Tile: Rectangle {
        property alias label: metric.label
        property alias value: metric.value
        property alias unit: metric.unit
        property alias iconName: metric.iconName
        property alias valueColor: metric.valueColor
        Layout.fillWidth: true
        implicitHeight: 66
        radius: Theme.radiusMd
        color: Theme.surfaceAlt
        border.color: Theme.border
        Metric {
            id: metric
            anchors.left: parent.left
            anchors.leftMargin: 14
            anchors.verticalCenter: parent.verticalCenter
            valueSize: 20
        }
    }

    // A list that keeps the newest row in view while scrolled to the end, and offers a
    // "Latest" button once the user scrolls up.
    component TailList: ListView {
        id: tail
        property bool followTail: true
        property bool scrollingToEnd: false
        function scrollToEnd() {
            scrollingToEnd = true
            positionViewAtEnd()
            scrollingToEnd = false
        }
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: AppScrollBar { }
        onCountChanged: if (followTail) Qt.callLater(tail.scrollToEnd)
        // a scroll by the user (drag, wheel, scroll bar) decides whether to keep following
        onContentYChanged: {
            const byUser = moving || dragging || (ScrollBar.vertical !== null && ScrollBar.vertical.pressed)
            if (!scrollingToEnd && byUser)
                followTail = atYEnd
        }

        AppButton {
            parent: tail
            z: 2
            visible: !tail.followTail && tail.count > 0
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.margins: 10
            size: "sm"
            variant: "primary"
            text: "Latest"
            iconName: "chevronDown"
            onClicked: {
                tail.followTail = true
                tail.scrollToEnd()
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 1
        spacing: 0

        // --- tab row and actions -------------------------------------------------------------
        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 44

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 6
                anchors.rightMargin: 8
                spacing: 6

                Tabs {
                    id: tabs
                    Layout.preferredHeight: 44
                    variant: "underline"
                    model: [
                        { "text": "TC-Basic", "icon": "gauge", "count": bridge.tcBasicData.length > 0 ? bridge.tcBasicData.length : undefined },
                        { "text": "TC-SC", "icon": "grid" },
                        { "text": "Process data", "icon": "list" },
                        { "text": "DDI traffic", "icon": "activity" },
                        { "text": "Event log", "icon": "terminal" }
                    ]
                    onActivated: if (root.collapsed) root.collapseToggled()
                }

                Item { Layout.fillWidth: true }

                // actions of the open tab
                AppButton {
                    visible: tabs.currentIndex === 1 && !root.collapsed
                    variant: "ghost"
                    size: "sm"
                    text: "Clear coverage"
                    iconName: "eraser"
                    onClicked: bridge.clearWorkedArea()
                }
                AppSwitch {
                    visible: tabs.currentIndex === 3 && !root.collapsed
                    text: "Live watch"
                    font.pixelSize: Theme.fontSmall
                    checked: bridge.liveDdiTrafficWatch
                    onToggled: bridge.setLiveDdiTrafficWatch(checked)
                    AppToolTip {
                        text: "Show the continuous TC / client DDI traffic"
                        visible: parent.hovered
                    }
                }
                AppButton {
                    visible: (tabs.currentIndex === 3 || tabs.currentIndex === 4) && !root.collapsed
                    variant: "ghost"
                    size: "sm"
                    text: "Clear"
                    iconName: "trash"
                    onClicked: tabs.currentIndex === 3 ? bridge.clearDdiTraffic() : bridge.clearLog()
                }

                Rectangle {
                    visible: !root.detached
                    Layout.preferredWidth: 1
                    Layout.preferredHeight: 20
                    color: Theme.border
                }
                AppButton {
                    visible: !root.detached
                    variant: "ghost"
                    size: "sm"
                    iconName: "externalLink"
                    tip: "Open in a window of its own"
                    onClicked: root.popOutRequested()
                }
                AppButton {
                    visible: !root.detached
                    variant: "ghost"
                    size: "sm"
                    iconName: root.collapsed ? "chevronUp" : "chevronDown"
                    tip: root.collapsed ? "Show the data" : "Fold the data away"
                    onClicked: root.collapseToggled()
                }
            }

            Divider {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
            }
        }

        StackLayout {
            visible: !root.collapsed
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: tabs.currentIndex

            // --- TC-Basic --------------------------------------------------------------------
            Item {
                EmptyState {
                    visible: bridge.tcBasicData.length === 0
                    anchors.centerIn: parent
                    width: Math.min(parent.width - 32, 420)
                    iconName: "gauge"
                    title: "No implement data yet"
                    text: "Select the client (its pool loads automatically), then start a task — the values appear as they arrive."
                }

                ListView {
                    id: basicList
                    visible: bridge.tcBasicData.length > 0
                    anchors.fill: parent
                    anchors.margins: 8
                    model: bridge.tcBasicData
                    clip: true
                    spacing: 2
                    boundsBehavior: Flickable.StopAtBounds
                    ScrollBar.vertical: AppScrollBar { }

                    delegate: Rectangle {
                        width: basicList.width
                        height: 38
                        radius: Theme.radiusSm
                        color: basicMouse.containsMouse ? Theme.surfaceHover
                                                        : (index % 2 ? Theme.alpha(Theme.surfaceAlt, 0.7) : Theme.alpha(Theme.surfaceAlt, 0))
                        MouseArea {
                            id: basicMouse
                            anchors.fill: parent
                            hoverEnabled: true
                        }
                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12
                            spacing: 12
                            Text {
                                Layout.fillWidth: true
                                text: modelData.label
                                font.family: Theme.fontFamily
                                font.pixelSize: Theme.fontBody
                                color: Theme.text
                                elide: Text.ElideRight
                            }
                            Text {
                                text: "DDI " + modelData.ddi + " · el " + modelData.element
                                font.family: Theme.monoFamily
                                font.pixelSize: Theme.fontCaption
                                color: Theme.textMuted
                            }
                            Row {
                                Layout.preferredWidth: 170
                                layoutDirection: Qt.RightToLeft
                                spacing: 5
                                Text {
                                    visible: modelData.hasValue
                                    anchors.baseline: basicValue.baseline
                                    text: modelData.unit
                                    font.family: Theme.fontFamily
                                    font.pixelSize: Theme.fontSmall
                                    color: Theme.textMuted
                                }
                                Text {
                                    id: basicValue
                                    text: modelData.hasValue ? Number(modelData.displayValue).toLocaleString(Qt.locale(), 'f', 2) : "waiting"
                                    font.family: modelData.hasValue ? Theme.monoFamily : Theme.fontFamily
                                    font.pixelSize: modelData.hasValue ? Theme.fontTitle : Theme.fontSmall
                                    font.weight: modelData.hasValue ? Font.DemiBold : Font.Normal
                                    color: modelData.hasValue ? Theme.accentText : Theme.textMuted
                                }
                            }
                        }
                    }
                }
            }

            // --- TC-SC -----------------------------------------------------------------------
            ScrollPage {
                padding: 14
                spacing: 14

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12
                    AppSwitch {
                        text: "Automatic section control"
                        checked: bridge.autoSectionControl
                        onToggled: bridge.setAutoSectionControl(checked)
                        AppToolTip {
                            text: "While a task is active, the TC switches the client to automatic and turns its sections on and off from the field boundary and the coverage."
                            visible: parent.hovered
                        }
                    }
                    StatusBadge {
                        Layout.maximumWidth: 420
                        text: bridge.sectionControlStatus
                        tone: bridge.sectionControlStatus.startsWith("Automatic") ? "success"
                              : (bridge.sectionControlStatus.startsWith("Ready") ? "info" : "neutral")
                        pulse: bridge.sectionControlStatus.startsWith("Automatic")
                    }
                    Item { Layout.fillWidth: true }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    Tile {
                        label: "Worked area"
                        iconName: "sprout"
                        value: bridge.workedAreaHa.toFixed(3)
                        unit: "ha"
                        valueColor: bridge.workedAreaHa > 0 ? Theme.accentText : Theme.text
                    }
                    Tile {
                        label: "Distance"
                        iconName: "route"
                        value: bridge.workedDistanceM.toFixed(1)
                        unit: "m"
                    }
                    Tile {
                        label: "Time"
                        iconName: "clock"
                        value: root.formatDuration(bridge.workedTimeSeconds)
                    }
                    Tile {
                        label: "Sections on"
                        iconName: "grid"
                        value: bridge.activeSectionCount + " / " + bridge.sectionCount
                        valueColor: bridge.activeSectionCount > 0 ? Theme.accentText : Theme.text
                    }
                }

                // one LED bar per boom, as wide as the boom, an LED per section
                BoomLedBars {
                    visible: bridge.booms.length > 0
                    Layout.fillWidth: true
                    barHeight: 26
                }

                // without a client DDOP: the sections of the manual section DDI
                ColumnLayout {
                    visible: bridge.booms.length === 0
                    Layout.fillWidth: true
                    spacing: 10

                    SectionLabel { text: "Sections from a process data DDI (no DDOP)" }
                    RowLayout {
                        spacing: 12
                        FormField {
                            label: "Section DDI"
                            AppSpinBox {
                                Layout.preferredWidth: 130
                                from: 0
                                to: 65535
                                value: bridge.sectionDdi
                                onValueModified: bridge.setSectionDdi(value)
                            }
                        }
                        FormField {
                            label: "Sections"
                            AppSpinBox {
                                Layout.preferredWidth: 112
                                from: 1
                                to: 96
                                value: bridge.sectionCount
                                onValueModified: bridge.setSectionCount(value)
                            }
                        }
                    }
                    Flow {
                        Layout.fillWidth: true
                        spacing: 5
                        Repeater {
                            model: bridge.sectionStates
                            delegate: Rectangle {
                                width: 44
                                height: 28
                                radius: 6
                                color: modelData ? Theme.ledOn : Theme.ledOff
                                border.color: modelData ? Theme.ledOnBorder : Theme.ledOffBorder
                                Text {
                                    anchors.centerIn: parent
                                    text: index + 1
                                    font.family: Theme.fontFamily
                                    font.pixelSize: Theme.fontSmall
                                    font.weight: Font.DemiBold
                                    color: modelData ? Theme.ledOnText : Theme.textMuted
                                }
                            }
                        }
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    SectionLabel { text: "Rate control" }
                    Text {
                        visible: bridge.rateSetpoints.length === 0
                        text: "The client's DDOP offers no rate setpoints."
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontSmall
                        color: Theme.textMuted
                    }
                    Repeater {
                        model: bridge.rateSetpoints
                        delegate: Rectangle {
                            Layout.fillWidth: true
                            implicitHeight: 48
                            radius: Theme.radiusMd
                            color: Theme.surfaceAlt
                            border.color: Theme.border
                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 12
                                anchors.rightMargin: 8
                                spacing: 12
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 0
                                    Text {
                                        Layout.fillWidth: true
                                        text: modelData.name !== "" ? modelData.name : "Rate setpoint"
                                        font.family: Theme.fontFamily
                                        font.pixelSize: Theme.fontBody
                                        font.weight: Font.DemiBold
                                        color: Theme.text
                                        elide: Text.ElideRight
                                    }
                                    Text {
                                        text: "DDI " + modelData.ddi + " · element " + modelData.element + " · raw DDOP unit, 0 sends nothing"
                                        font.family: Theme.fontFamily
                                        font.pixelSize: Theme.fontCaption
                                        color: Theme.textMuted
                                    }
                                }
                                AppSpinBox {
                                    Layout.preferredWidth: 170
                                    from: 0
                                    to: 2000000000
                                    value: modelData.target
                                    onValueModified: bridge.setRateTarget(index, value)
                                }
                            }
                        }
                    }
                }
            }

            // --- raw process data ------------------------------------------------------------
            ColumnLayout {
                spacing: 0

                RowLayout {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 30
                    Layout.leftMargin: 20
                    Layout.rightMargin: 20
                    spacing: 0
                    ColumnTitle { Layout.preferredWidth: 80; text: "Address" }
                    ColumnTitle { Layout.preferredWidth: 100; text: "DDI" }
                    ColumnTitle { Layout.preferredWidth: 100; text: "Element" }
                    ColumnTitle { Layout.preferredWidth: 160; text: "Value" }
                    ColumnTitle { Layout.fillWidth: true; text: "Time" }
                }
                Divider { Layout.fillWidth: true }

                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    EmptyState {
                        visible: valueList.count === 0
                        anchors.centerIn: parent
                        width: Math.min(parent.width - 32, 420)
                        iconName: "list"
                        title: "No process data yet"
                        text: "Every value a client sends appears here, one row per client, DDI and element."
                    }

                    ListView {
                        id: valueList
                        anchors.fill: parent
                        anchors.margins: 8
                        model: valueModel
                        clip: true
                        boundsBehavior: Flickable.StopAtBounds
                        ScrollBar.vertical: AppScrollBar { }

                        delegate: Rectangle {
                            width: valueList.width
                            height: 28
                            radius: Theme.radiusSm
                            color: valueMouse.containsMouse ? Theme.surfaceHover
                                                            : (index % 2 ? Theme.alpha(Theme.surfaceAlt, 0.7) : Theme.alpha(Theme.surfaceAlt, 0))
                            MouseArea {
                                id: valueMouse
                                anchors.fill: parent
                                hoverEnabled: true
                            }
                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 12
                                anchors.rightMargin: 12
                                spacing: 0
                                Cell { Layout.preferredWidth: 80; text: valueAddress; color: Theme.textSecondary }
                                Cell { Layout.preferredWidth: 100; text: valueDdi }
                                Cell { Layout.preferredWidth: 100; text: valueElement; color: Theme.textSecondary }
                                Cell { Layout.preferredWidth: 160; text: valueContent; color: Theme.accentText; font.weight: Font.DemiBold }
                                Cell { Layout.fillWidth: true; text: valueTime; color: Theme.textMuted }
                            }
                        }
                    }
                }
            }

            // --- DDI traffic -----------------------------------------------------------------
            ColumnLayout {
                spacing: 0

                RowLayout {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 30
                    Layout.leftMargin: 20
                    Layout.rightMargin: 20
                    spacing: 0
                    ColumnTitle { Layout.preferredWidth: 96; text: "Time" }
                    ColumnTitle { Layout.preferredWidth: 104; text: "Direction" }
                    ColumnTitle { Layout.preferredWidth: 150; text: "Command" }
                    ColumnTitle { Layout.preferredWidth: 56; text: "Addr" }
                    ColumnTitle { Layout.preferredWidth: 76; text: "DDI" }
                    ColumnTitle { Layout.preferredWidth: 64; text: "Elem" }
                    ColumnTitle { Layout.preferredWidth: 100; text: "Value" }
                    ColumnTitle { Layout.fillWidth: true; text: "Detail" }
                }
                Divider { Layout.fillWidth: true }

                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    EmptyState {
                        visible: trafficList.count === 0
                        anchors.centerIn: parent
                        width: Math.min(parent.width - 32, 420)
                        iconName: "activity"
                        title: bridge.liveDdiTrafficWatch ? "No DDI traffic yet" : "Live watch is off"
                        text: bridge.liveDdiTrafficWatch ? "Process data commands between this TC and its clients appear here as they happen."
                                                         : "Switch on Live watch to see the process data commands between this TC and its clients."
                    }

                    TailList {
                        id: trafficList
                        anchors.fill: parent
                        anchors.margins: 8
                        model: ddiTrafficModel

                        delegate: Rectangle {
                            id: trafficRow
                            readonly property bool fromTc: trafficDirection.indexOf("TC") === 0
                            readonly property color directionColor: fromTc ? Theme.fromTc : Theme.fromClient
                            width: trafficList.width
                            height: 26
                            radius: Theme.radiusSm
                            color: trafficMouse.containsMouse ? Theme.surfaceHover
                                                              : (index % 2 ? Theme.alpha(Theme.surfaceAlt, 0.7) : Theme.alpha(Theme.surfaceAlt, 0))
                            MouseArea {
                                id: trafficMouse
                                anchors.fill: parent
                                hoverEnabled: true
                            }
                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 12
                                anchors.rightMargin: 12
                                spacing: 0
                                Cell { Layout.preferredWidth: 96; text: trafficTime; color: Theme.textMuted }
                                Item {
                                    Layout.preferredWidth: 104
                                    Layout.fillHeight: true
                                    Rectangle {
                                        anchors.verticalCenter: parent.verticalCenter
                                        width: directionText.implicitWidth + 14
                                        height: 18
                                        radius: 9
                                        color: Theme.alpha(trafficRow.directionColor, 0.15)
                                        Text {
                                            id: directionText
                                            anchors.centerIn: parent
                                            text: trafficDirection
                                            font.family: Theme.fontFamily
                                            font.pixelSize: Theme.fontCaption
                                            font.weight: Font.DemiBold
                                            color: trafficRow.directionColor
                                        }
                                    }
                                }
                                Cell { Layout.preferredWidth: 150; text: trafficCommand; font.family: Theme.fontFamily }
                                Cell { Layout.preferredWidth: 56; text: trafficAddress; color: Theme.textSecondary }
                                Cell { Layout.preferredWidth: 76; text: trafficDdi }
                                Cell { Layout.preferredWidth: 64; text: trafficElement; color: Theme.textSecondary }
                                Cell { Layout.preferredWidth: 100; text: trafficValue; color: Theme.accentText; font.weight: Font.DemiBold }
                                Cell { Layout.fillWidth: true; text: trafficDetail; color: Theme.textMuted; font.family: Theme.fontFamily }
                            }
                        }
                    }
                }
            }

            // --- event log: what the server did and saw, newest at the bottom -----------------
            Item {
                TailList {
                    id: logList
                    anchors.fill: parent
                    anchors.margins: 8
                    model: logModel

                    delegate: Item {
                        id: logRow

                        readonly property var parts: {
                            const match = /^\[([^\]]+)\]\s?(.*)$/.exec(logLine)
                            return match ? { tag: match[1], message: match[2] } : { tag: "", message: logLine }
                        }
                        readonly property string tone: root.tagTones[parts.tag] || "neutral"
                        readonly property bool problem: /warn|fail|error|timeout|could not|cannot/i.test(logLine)

                        width: logList.width
                        height: Math.max(24, message.implicitHeight + 6)

                        Rectangle {
                            anchors.fill: parent
                            radius: Theme.radiusSm
                            color: logRow.problem ? Theme.alpha(Theme.warning, 0.08)
                                                  : (logMouse.containsMouse ? Theme.surfaceHover : Theme.alpha(Theme.surfaceHover, 0))
                        }
                        MouseArea {
                            id: logMouse
                            anchors.fill: parent
                            hoverEnabled: true
                        }

                        Rectangle {
                            id: tagChip
                            visible: logRow.parts.tag !== ""
                            x: 8
                            y: 4
                            width: Math.max(44, tagText.implicitWidth + 12)
                            height: 17
                            radius: 4
                            color: Theme.alpha(root.toneColor(logRow.tone), 0.14)
                            Text {
                                id: tagText
                                anchors.centerIn: parent
                                text: logRow.parts.tag
                                font.family: Theme.monoFamily
                                font.pixelSize: 10
                                font.weight: Font.DemiBold
                                color: logRow.tone === "neutral" ? Theme.textSecondary : root.toneColor(logRow.tone)
                            }
                        }
                        Text {
                            id: message
                            x: tagChip.visible ? tagChip.x + tagChip.width + 10 : 8
                            y: 3
                            width: parent.width - x - 8
                            text: logRow.parts.message
                            font.family: Theme.monoFamily
                            font.pixelSize: Theme.fontSmall
                            color: logRow.problem ? Theme.warning : Theme.textSecondary
                            wrapMode: Text.Wrap
                        }
                    }
                }
            }
        }
    }
}
