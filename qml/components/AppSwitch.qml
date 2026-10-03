import QtQuick
import QtQuick.Templates as T
import AgIsoTc 1.0

// On/off switch with its label on the right.
T.Switch {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding,
                             implicitIndicatorHeight + topPadding + bottomPadding)

    padding: 4
    leftPadding: 0
    spacing: 10
    hoverEnabled: true
    opacity: enabled ? 1 : 0.45

    font.family: Theme.fontFamily
    font.pixelSize: Theme.fontBody

    indicator: Rectangle {
        implicitWidth: 34
        implicitHeight: 20
        x: control.leftPadding
        y: control.topPadding + (control.availableHeight - height) / 2
        radius: height / 2
        color: control.checked ? (control.hovered ? Theme.accentHover : Theme.accent)
                               : (control.hovered ? Theme.borderStrong : Theme.surfacePressed)
        border.width: control.checked ? 0 : 1
        border.color: Theme.borderStrong
        Behavior on color { ColorAnimation { duration: Theme.normal } }

        Rectangle {
            width: 14
            height: 14
            radius: 7
            y: 3
            x: control.checked ? parent.width - width - 3 : 3
            color: control.checked ? "#ffffff" : Theme.textSecondary
            Behavior on x { NumberAnimation { duration: Theme.normal; easing.type: Easing.OutCubic } }
        }

        Rectangle {
            anchors.fill: parent
            anchors.margins: -3
            radius: height / 2
            color: "transparent"
            border.width: 2
            border.color: Theme.alpha(Theme.accent, 0.55)
            visible: control.visualFocus
        }
    }

    contentItem: Text {
        leftPadding: control.text !== "" ? control.indicator.width + control.spacing : control.indicator.width
        text: control.text
        font: control.font
        color: Theme.text
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
}
