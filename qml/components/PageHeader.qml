import QtQuick
import QtQuick.Layouts
import AgIsoTc 1.0

// The title and subtitle on top of a side panel page.
ColumnLayout {
    id: root

    property string title
    property string subtitle

    spacing: 3

    Text {
        Layout.fillWidth: true
        text: root.title
        font.family: Theme.fontFamily
        font.pixelSize: Theme.fontHeading
        font.weight: Font.DemiBold
        color: Theme.text
        elide: Text.ElideRight
    }
    Text {
        visible: root.subtitle !== ""
        Layout.fillWidth: true
        text: root.subtitle
        font.family: Theme.fontFamily
        font.pixelSize: Theme.fontSmall
        color: Theme.textMuted
        wrapMode: Text.Wrap
    }
}
