import QtQuick
import QtQuick.Layouts
import AgIsoTc 1.0

// What a view shows while it has nothing to show: an icon, a title, a line of help, and
// optional actions (children).
ColumnLayout {
    id: root

    property string iconName: "info"
    property string title
    property string text
    default property alias actions: actionRow.data

    spacing: 8

    Rectangle {
        Layout.alignment: Qt.AlignHCenter
        implicitWidth: 44
        implicitHeight: 44
        radius: 12
        color: Theme.surfaceAlt
        border.color: Theme.border
        Icon {
            anchors.centerIn: parent
            name: root.iconName
            size: 20
            color: Theme.textMuted
        }
    }

    Text {
        Layout.fillWidth: true
        horizontalAlignment: Text.AlignHCenter
        text: root.title
        font.family: Theme.fontFamily
        font.pixelSize: Theme.fontTitle
        font.weight: Font.DemiBold
        color: Theme.text
        wrapMode: Text.Wrap
    }

    Text {
        visible: root.text !== ""
        Layout.fillWidth: true
        Layout.maximumWidth: 340
        Layout.alignment: Qt.AlignHCenter
        horizontalAlignment: Text.AlignHCenter
        text: root.text
        font.family: Theme.fontFamily
        font.pixelSize: Theme.fontSmall
        color: Theme.textMuted
        wrapMode: Text.Wrap
        lineHeight: 1.15
    }

    Row {
        id: actionRow
        Layout.alignment: Qt.AlignHCenter
        Layout.topMargin: children.length > 0 ? 4 : 0
        spacing: 8
    }
}
