import QtQuick
import QtQuick.Templates as T
import AgIsoTc 1.0

// Thin overlay scroll bar: shown while scrolling or hovered, widens under the mouse.
T.ScrollBar {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    padding: 2
    minimumSize: orientation === Qt.Horizontal ? height / width : width / height
    visible: policy !== T.ScrollBar.AlwaysOff
    hoverEnabled: true

    contentItem: Rectangle {
        id: thumb
        readonly property real thickness: control.hovered || control.pressed ? 8 : 5
        implicitWidth: control.interactive ? thickness : 3
        implicitHeight: control.interactive ? thickness : 3
        radius: Math.min(width, height) / 2
        color: control.pressed || control.hovered ? Theme.scrollThumbHover : Theme.scrollThumb
        opacity: 0
        Behavior on implicitWidth { NumberAnimation { duration: Theme.fast } }
        Behavior on implicitHeight { NumberAnimation { duration: Theme.fast } }

        states: State {
            name: "active"
            when: control.policy === T.ScrollBar.AlwaysOn || (control.active && control.size < 1.0)
            PropertyChanges { target: thumb; opacity: 1 }
        }
        transitions: Transition {
            from: "active"
            SequentialAnimation {
                PauseAnimation { duration: 600 }
                NumberAnimation { target: thumb; property: "opacity"; to: 0; duration: 250 }
            }
        }
    }
}
