import QtQuick
import QtQuick.Layouts
import AgIsoTc 1.0
import "components"

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
    property bool showLabels: true

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

    spacing: compact ? 5 : 10

    // one tooltip for all LEDs, moved to the LED under the mouse
    AppToolTip {
        id: ledTip
        delay: 150
    }

    Repeater {
        model: root.booms
        delegate: ColumnLayout {
            id: boomColumn
            required property var modelData
            Layout.fillWidth: true
            spacing: 4

            RowLayout {
                visible: root.showLabels
                Layout.fillWidth: true
                spacing: 8
                Text {
                    text: boomColumn.modelData.name
                    font.family: Theme.fontFamily
                    font.pixelSize: root.compact ? Theme.fontCaption : Theme.fontSmall
                    font.weight: Font.DemiBold
                    color: Theme.text
                }
                Text {
                    text: boomColumn.modelData.onCount + " / " + boomColumn.modelData.count + " on"
                    font.family: Theme.fontFamily
                    font.pixelSize: root.compact ? Theme.fontCaption : Theme.fontSmall
                    font.weight: Font.DemiBold
                    color: boomColumn.modelData.onCount > 0 ? Theme.accentText : Theme.textMuted
                }
                Item { Layout.fillWidth: true }
                Text {
                    visible: !root.compact
                    text: boomColumn.modelData.widthM.toFixed(2) + " m  ·  element " + boomColumn.modelData.element
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontCaption
                    color: Theme.textMuted
                }
            }

            Item {
                id: track
                Layout.fillWidth: true
                Layout.preferredHeight: root.barHeight
                readonly property real metre: width / root.spanM

                // The boom: a rail as wide as its sections reach.
                Rectangle {
                    x: (boomColumn.modelData.left - root.spanLeft) * track.metre
                    width: Math.max(4, boomColumn.modelData.widthM * track.metre)
                    height: parent.height
                    radius: Math.min(6, height / 3)
                    color: Theme.ledRail
                    border.color: Theme.border
                }

                Repeater {
                    model: boomColumn.modelData.sections
                    delegate: Rectangle {
                        id: led
                        required property var modelData
                        readonly property real gap: Math.min(2, modelData.width * track.metre * 0.08)
                        x: (modelData.left - root.spanLeft) * track.metre + gap
                        y: 2
                        width: Math.max(2, modelData.width * track.metre - 2 * gap)
                        height: track.height - 4
                        radius: Math.min(4, width / 3)
                        color: modelData.on ? Theme.ledOn : Theme.ledOff
                        border.color: modelData.on ? Theme.ledOnBorder : Theme.ledOffBorder
                        Behavior on color { ColorAnimation { duration: Theme.fast } }

                        // a soft glow while lit
                        Rectangle {
                            anchors.centerIn: parent
                            visible: led.modelData.on
                            width: parent.width + 4
                            height: parent.height + 4
                            radius: parent.radius + 2
                            color: "transparent"
                            border.color: Theme.alpha(Theme.ledOn, 0.35)
                            border.width: 2
                        }

                        Text {
                            anchors.centerIn: parent
                            visible: led.width >= 16 && led.height >= 12
                            text: led.modelData.number
                            font.family: Theme.fontFamily
                            font.pixelSize: Math.min(10, led.height - 4)
                            font.weight: Font.DemiBold
                            color: led.modelData.on ? Theme.ledOnText : Theme.textMuted
                        }

                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: true
                            onContainsMouseChanged: {
                                if (containsMouse) {
                                    ledTip.parent = led
                                    ledTip.text = boomColumn.modelData.name + ", section " + led.modelData.number
                                                  + " (element " + led.modelData.element + "): "
                                                  + (led.modelData.on ? "on" : "off") + ", "
                                                  + led.modelData.width.toFixed(2) + " m"
                                    ledTip.open()
                                } else if (ledTip.parent === led) {
                                    ledTip.close()
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
