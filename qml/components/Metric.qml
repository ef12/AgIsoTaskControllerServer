import QtQuick
import QtQuick.Layouts
import AgIsoTc 1.0

// A labelled figure: a small caption over a large value and its unit.
ColumnLayout {
    id: root

    property string label
    property string value
    property string unit
    property string iconName
    property color valueColor: Theme.text
    property int valueSize: 18

    spacing: 2

    RowLayout {
        Layout.fillWidth: true
        spacing: 5
        Icon {
            visible: root.iconName !== ""
            name: root.iconName
            size: 12
            color: Theme.textMuted
        }
        Text {
            text: root.label
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontCaption
            font.weight: Font.DemiBold
            font.letterSpacing: 0.5
            font.capitalization: Font.AllUppercase
            color: Theme.textMuted
        }
        // fills, so that the metric can stretch when its layout asks it to
        Item { Layout.fillWidth: true }
    }

    Row {
        spacing: 4
        Text {
            id: valueText
            text: root.value
            font.family: Theme.fontFamily
            font.pixelSize: root.valueSize
            font.weight: Font.DemiBold
            color: root.valueColor
        }
        Text {
            visible: root.unit !== ""
            anchors.baseline: valueText.baseline
            text: root.unit
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSmall
            color: Theme.textMuted
        }
    }
}
