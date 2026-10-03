import QtQuick
import QtQuick.Layouts
import AgIsoTc 1.0
import "components"

// Compact TC-SC section strip for the map window: one LED bar per boom (BoomLedBars,
// bridge.booms), or without a client DDOP one block per section (bridge.sectionStates), plus
// the worked area.
GlassPanel {
    id: root

    implicitHeight: column.implicitHeight + 24

    ColumnLayout {
        id: column
        anchors.fill: parent
        anchors.margins: 12
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            spacing: 10
            SectionLabel { text: "Sections" }
            Text {
                text: bridge.activeSectionCount + " / " + bridge.sectionCount + " on"
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSmall
                font.weight: Font.DemiBold
                color: bridge.activeSectionCount > 0 ? Theme.accentText : Theme.textMuted
            }
            Item { Layout.fillWidth: true }
            Icon {
                name: "sprout"
                size: 14
                color: Theme.textMuted
            }
            Text {
                text: bridge.workedAreaHa.toFixed(3) + " ha worked"
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSmall
                font.weight: Font.DemiBold
                color: bridge.workedAreaHa > 0 ? Theme.accentText : Theme.textSecondary
            }
        }

        BoomLedBars {
            visible: bridge.booms.length > 0
            Layout.fillWidth: true
            compact: true
            barHeight: 18
        }

        Flow {
            visible: bridge.booms.length === 0
            Layout.fillWidth: true
            spacing: 4
            Repeater {
                model: bridge.sectionStates
                delegate: Rectangle {
                    width: 24
                    height: 22
                    radius: 5
                    color: modelData ? Theme.ledOn : Theme.ledOff
                    border.color: modelData ? Theme.ledOnBorder : Theme.ledOffBorder
                    Text {
                        anchors.centerIn: parent
                        text: index + 1
                        font.family: Theme.fontFamily
                        font.pixelSize: 10
                        font.weight: Font.DemiBold
                        color: modelData ? Theme.ledOnText : Theme.textMuted
                    }
                }
            }
        }
    }
}
