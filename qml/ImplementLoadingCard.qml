import QtQuick
import QtQuick.Layouts
import AgIsoTc 1.0
import "components"

// How far an implement is in connecting to this TC (bridge.implementLoading), from the moment it
// appears on the bus until it is built: its start-up wait (ISO 11783-10: 6 s, then the next TC
// status message), the connection, the DDOP upload and the geometry values. Fades out when ready.
GlassPanel {
    id: card

    readonly property var loading: bridge.implementLoading
    readonly property bool active: !!loading.active
    readonly property int step: loading.step || 0
    readonly property real progress: loading.progress || 0
    readonly property bool ready: step === 5

    readonly property var steps: [
        { number: 1, title: "Starting up" },
        { number: 2, title: "Connecting to the TC" },
        { number: 3, title: "Uploading the device descriptor" },
        { number: 4, title: "Building the implement" }
    ]

    function kilobytes(bytes) {
        return (bytes / 1024).toFixed(1)
    }

    function stepDetail(number) {
        if (number !== step)
            return number === 3 && step > 3 && loading.uploadSkipped ? "stored copy" : ""
        switch (number) {
        case 1: return "waits about 6 s before it talks to a TC"
        case 2: return "version and stored pool"
        case 3: return loading.transferBytes > 0 ? kilobytes(loading.receivedBytes) + " / " + kilobytes(loading.transferBytes) + " kB"
                                                 : "starting the transfer"
        case 4: return loading.geometryTotal > 0 ? "geometry " + loading.geometryReceived + " / " + loading.geometryTotal
                                                 : "reading the geometry"
        }
        return ""
    }

    implicitWidth: 380
    implicitHeight: column.implicitHeight + 28
    opacity: active ? 1 : 0
    visible: opacity > 0.01
    Behavior on opacity { NumberAnimation { duration: Theme.slow } }

    ColumnLayout {
        id: column
        anchors.fill: parent
        anchors.margins: 14
        spacing: 12

        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            Rectangle {
                implicitWidth: 36
                implicitHeight: 36
                radius: 10
                color: Theme.alpha(Theme.accent, 0.14)
                Spinner {
                    anchors.centerIn: parent
                    visible: !card.ready
                    size: 20
                }
                Icon {
                    anchors.centerIn: parent
                    visible: card.ready
                    name: "check"
                    size: 18
                    color: Theme.accentText
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 1
                Text {
                    Layout.fillWidth: true
                    text: card.ready ? "Implement ready" : "Connecting an implement"
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontTitle
                    font.weight: Font.DemiBold
                    color: Theme.text
                    elide: Text.ElideRight
                }
                Text {
                    Layout.fillWidth: true
                    text: "SA " + card.loading.address + (card.loading.name ? "  ·  " + card.loading.name : "")
                          + "  ·  " + Math.floor((card.loading.elapsedMs || 0) / 1000) + " s"
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSmall
                    color: Theme.textMuted
                    elide: Text.ElideRight
                }
            }

            Text {
                text: Math.round(card.progress * 100) + "%"
                font.family: Theme.fontFamily
                font.pixelSize: 20
                font.weight: Font.DemiBold
                color: card.ready ? Theme.accentText : Theme.text
            }
        }

        // the bar
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 8
            radius: 4
            color: Theme.surfacePressed
            clip: true

            Rectangle {
                id: fill
                width: parent.width * Math.max(0.02, card.progress)
                height: parent.height
                radius: 4
                color: Theme.accent
                Behavior on width { NumberAnimation { duration: 400; easing.type: Easing.OutCubic } }

                // a glint running along the fill while it works
                Rectangle {
                    id: glint
                    visible: !card.ready
                    width: 60
                    height: parent.height
                    gradient: Gradient {
                        orientation: Gradient.Horizontal
                        GradientStop { position: 0; color: Qt.rgba(1, 1, 1, 0) }
                        GradientStop { position: 0.5; color: Qt.rgba(1, 1, 1, 0.35) }
                        GradientStop { position: 1; color: Qt.rgba(1, 1, 1, 0) }
                    }
                    NumberAnimation on x {
                        running: card.active && !card.ready
                        from: -glint.width
                        to: fill.width
                        duration: 1400
                        loops: Animation.Infinite
                    }
                }
            }
        }

        // the steps
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 6

            Repeater {
                model: card.steps
                delegate: RowLayout {
                    id: row
                    readonly property bool done: card.step > modelData.number
                    readonly property bool current: card.step === modelData.number
                    Layout.fillWidth: true
                    spacing: 10

                    Item {
                        implicitWidth: 18
                        implicitHeight: 18
                        Rectangle {
                            anchors.fill: parent
                            radius: 9
                            color: row.done ? Theme.accent : "transparent"
                            border.width: row.done ? 0 : 1.5
                            border.color: row.current ? Theme.accent : Theme.borderStrong
                        }
                        Icon {
                            anchors.centerIn: parent
                            visible: row.done
                            name: "check"
                            size: 12
                            stroke: 3
                            color: Theme.onAccent
                        }
                        PulseDot {
                            anchors.centerIn: parent
                            visible: row.current
                            size: 6
                            color: Theme.accent
                            pulse: true
                        }
                    }
                    Text {
                        Layout.fillWidth: true
                        text: modelData.title
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontSmall
                        font.weight: row.current ? Font.DemiBold : Font.Normal
                        color: row.current ? Theme.text : (row.done ? Theme.textSecondary : Theme.textMuted)
                        elide: Text.ElideRight
                    }
                    Text {
                        text: card.stepDetail(modelData.number)
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontCaption
                        color: Theme.textMuted
                    }
                }
            }
        }
    }
}
