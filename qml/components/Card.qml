import QtQuick
import QtQuick.Layouts
import AgIsoTc 1.0

// A raised surface with an optional header: an icon tile, a title, a subtitle and trailing
// actions (`actions: [ ... ]`). Children are laid out in a column and may use Layout properties.
Rectangle {
    id: root

    property string title: ""
    property string subtitle: ""
    property string iconName: ""
    property color iconColor: Theme.accentText
    property int padding: Theme.s4
    property int spacing: Theme.s3
    property alias actions: actionRow.data
    default property alias content: body.data

    implicitWidth: column.implicitWidth + 2 * padding
    implicitHeight: column.implicitHeight + 2 * padding
    radius: Theme.radiusLg
    color: Theme.surfaceAlt
    border.color: Theme.border

    ColumnLayout {
        id: column
        anchors.fill: parent
        anchors.margins: root.padding
        spacing: root.spacing

        RowLayout {
            visible: root.title !== ""
            Layout.fillWidth: true
            spacing: 10

            Rectangle {
                visible: root.iconName !== ""
                Layout.alignment: Qt.AlignTop
                implicitWidth: 30
                implicitHeight: 30
                radius: 8
                color: Theme.alpha(root.iconColor, 0.13)
                Icon {
                    anchors.centerIn: parent
                    name: root.iconName
                    size: 16
                    color: root.iconColor
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 1
                Text {
                    Layout.fillWidth: true
                    text: root.title
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontTitle
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

            Row {
                id: actionRow
                Layout.alignment: Qt.AlignTop
                spacing: 2
            }
        }

        ColumnLayout {
            id: body
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: root.spacing
        }
    }
}
