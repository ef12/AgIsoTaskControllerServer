import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// The TC-SC sections as one LED bar per boom (bridge.booms). Every bar is drawn to the same
// scale and where its boom is across the implement, so a bar is as wide as its boom and each
// LED as wide as its section: a boom of 31 rows shows 31 narrow LEDs, a boom of 2 sections
// under it 2 wide ones. An LED is lit while its section is on (as the client reports it, or as
// the TC commands it to a client that reports nothing).
ColumnLayout {
    id: root

    property var booms: bridge.booms
    property int barHeight: 22
    property bool compact: false

    // The extent of all booms across the implement, in metres right of the connector.
    readonly property real spanLeft: {
        let value = 0
        for (let i = 0; i < booms.length; ++i) value = (i === 0) ? booms[i].left : Math.min(value, booms[i].left)
        return value
    }
    readonly property real spanRight: {
        let value = 1
        for (let i = 0; i < booms.length; ++i) value = (i === 0) ? booms[i].right : Math.max(value, booms[i].right)
        return value
    }
    readonly property real spanM: Math.max(0.1, spanRight - spanLeft)

    spacing: compact ? 4 : 8

    Repeater {
        model: root.booms
        delegate: ColumnLayout {
            id: boomColumn
            required property var modelData
            Layout.fillWidth: true
            spacing: 3

            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                Label {
                    text: boomColumn.modelData.name
                    color: "#dfe6ee"
                    font.bold: true
                    font.pixelSize: root.compact ? 11 : 12
                }
                Label {
                    text: boomColumn.modelData.onCount + " of " + boomColumn.modelData.count + " ON"
                    color: boomColumn.modelData.onCount > 0 ? "#3ddc6e" : "#8995a3"
                    font.pixelSize: root.compact ? 11 : 12
                }
                Label {
                    visible: !root.compact
                    text: boomColumn.modelData.widthM.toFixed(2) + " m, element " + boomColumn.modelData.element
                    color: "#8995a3"
                    font.pixelSize: 11
                }
                Item { Layout.fillWidth: true }
            }

            Item {
                id: track
                Layout.fillWidth: true
                Layout.preferredHeight: root.barHeight
                readonly property real metre: width / root.spanM

                // The boom: a bar as wide as its sections reach.
                Rectangle {
                    x: (boomColumn.modelData.left - root.spanLeft) * track.metre
                    width: Math.max(4, boomColumn.modelData.widthM * track.metre)
                    height: parent.height
                    radius: 4
                    color: "#161b21"
                    border.color: "#3a4048"
                }

                Repeater {
                    model: boomColumn.modelData.sections
                    delegate: Rectangle {
                        id: led
                        required property var modelData
                        readonly property real gap: Math.min(2, modelData.width * track.metre * 0.08)
                        x: (modelData.left - root.spanLeft) * track.metre + gap
                        y: 3
                        width: Math.max(2, modelData.width * track.metre - 2 * gap)
                        height: track.height - 6
                        radius: Math.min(3, width / 3)
                        color: modelData.on ? "#3ddc6e" : "#28313a"
                        border.color: modelData.on ? "#b8f5c9" : "#4b5560"
                        border.width: 1

                        // a soft glow while lit
                        Rectangle {
                            anchors.centerIn: parent
                            visible: led.modelData.on
                            width: parent.width + 4
                            height: parent.height + 4
                            radius: parent.radius + 2
                            color: "transparent"
                            border.color: "#553ddc6e"
                            border.width: 2
                        }

                        Text {
                            anchors.centerIn: parent
                            visible: led.width >= 16
                            text: led.modelData.number
                            font.pixelSize: Math.min(10, led.height - 6)
                            color: led.modelData.on ? "#0d2812" : "#8995a3"
                        }

                        MouseArea {
                            id: hover
                            anchors.fill: parent
                            hoverEnabled: true
                        }
                        ToolTip.visible: hover.containsMouse
                        ToolTip.text: boomColumn.modelData.name + ", section " + led.modelData.number
                                      + " (element " + led.modelData.element + "): "
                                      + (led.modelData.on ? "ON" : "OFF") + ", " + led.modelData.width.toFixed(2) + " m"
                    }
                }
            }
        }
    }
}
