import QtQuick
import QtQuick.Effects
import AgIsoTc 1.0

// A short notice that slides in at the top centre of its parent and leaves by itself.
Item {
    id: root

    property string iconName: "info"
    property string tone: "info"
    property string text
    property int duration: 3000
    property int topOffset: 16
    property bool shown: false

    function show(message) {
        text = message
        shown = true
        hideTimer.restart()
    }

    readonly property color toneColor: Theme.tone(tone)

    anchors.horizontalCenter: parent.horizontalCenter
    y: shown ? topOffset : -height - 12
    width: body.implicitWidth
    height: body.implicitHeight
    opacity: shown ? 1 : 0
    visible: opacity > 0
    z: 100
    Behavior on y { NumberAnimation { duration: Theme.slow; easing.type: Easing.OutCubic } }
    Behavior on opacity { NumberAnimation { duration: Theme.normal } }

    Timer {
        id: hideTimer
        interval: root.duration
        onTriggered: root.shown = false
    }

    Rectangle {
        id: body
        implicitWidth: row.implicitWidth + 28
        implicitHeight: 48
        radius: Theme.radiusLg
        color: Theme.popupBg
        border.color: Theme.alpha(root.toneColor, 0.45)
        layer.enabled: true
        layer.effect: MultiEffect {
            shadowEnabled: true
            shadowColor: Theme.shadow
            shadowBlur: 0.8
            shadowVerticalOffset: 8
        }

        Row {
            id: row
            anchors.centerIn: parent
            spacing: 10

            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                width: 28
                height: 28
                radius: 8
                color: Theme.alpha(root.toneColor, 0.16)
                Icon {
                    anchors.centerIn: parent
                    name: root.iconName
                    size: 16
                    color: root.toneColor
                }
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: root.text
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontBody
                font.weight: Font.DemiBold
                color: Theme.text
            }
        }
    }
}
