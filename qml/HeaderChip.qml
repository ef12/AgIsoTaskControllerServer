import QtQuick
import QtQuick.Layouts
import AgIsoTc 1.0
import "components"

// A clickable status summary in the header: a status dot, a title and a detail line.
Rectangle {
    id: root

    property string title
    property string detail
    property string tone: "neutral"
    property bool pulse: false
    property string tip: ""

    signal clicked()

    implicitHeight: 40
    implicitWidth: row.implicitWidth + 24
    radius: 10
    color: mouse.pressed ? Theme.surfacePressed : (mouse.containsMouse ? Theme.surfaceHover : Theme.surface)
    border.color: Theme.border
    Behavior on color { ColorAnimation { duration: Theme.fast } }

    RowLayout {
        id: row
        anchors.verticalCenter: parent.verticalCenter
        x: 12
        spacing: 10

        PulseDot {
            color: Theme.tone(root.tone)
            pulse: root.pulse
        }

        ColumnLayout {
            spacing: 0
            Text {
                text: root.title
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSmall
                font.weight: Font.DemiBold
                color: Theme.text
            }
            Text {
                Layout.maximumWidth: 280
                text: root.detail
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontCaption
                color: Theme.textMuted
                elide: Text.ElideRight
            }
        }

        Icon {
            name: "chevronRight"
            size: 14
            color: mouse.containsMouse ? Theme.textSecondary : Theme.textDisabled
        }
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }

    AppToolTip {
        text: root.tip
        visible: root.tip !== "" && mouse.containsMouse
    }
}
