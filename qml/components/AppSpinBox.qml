import QtQuick
import QtQuick.Templates as T
import AgIsoTc 1.0

// Numeric stepper: [ − | value | + ] in one field. Editable by default.
T.SpinBox {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            contentItem.implicitWidth + leftPadding + rightPadding)
    implicitHeight: Theme.controlHeight

    leftPadding: (down.indicator ? down.indicator.width : 0) + 4
    rightPadding: (up.indicator ? up.indicator.width : 0) + 4
    editable: true
    hoverEnabled: true
    opacity: enabled ? 1 : 0.5

    font.family: Theme.fontFamily
    font.pixelSize: Theme.fontBody

    validator: IntValidator {
        locale: control.locale.name
        bottom: Math.min(control.from, control.to)
        top: Math.max(control.from, control.to)
    }

    contentItem: TextInput {
        z: 2
        text: control.displayText
        clip: width < implicitWidth
        font: control.font
        color: Theme.text
        selectionColor: Theme.selection
        selectedTextColor: Theme.text
        horizontalAlignment: Qt.AlignHCenter
        verticalAlignment: Qt.AlignVCenter
        readOnly: !control.editable
        validator: control.validator
        inputMethodHints: control.inputMethodHints
        selectByMouse: true
    }

    down.indicator: Rectangle {
        x: 3
        y: 3
        implicitWidth: 26
        height: control.height - 6
        radius: Theme.radiusSm
        color: control.down.pressed ? Theme.surfacePressed
                                    : (control.down.hovered && enabled ? Theme.surfaceHover : Theme.alpha(Theme.surfaceHover, 0))
        Icon {
            anchors.centerIn: parent
            name: "minus"
            size: 14
            color: control.down.hovered ? Theme.text : Theme.textMuted
            opacity: parent.enabled ? 1 : 0.35
        }
    }

    up.indicator: Rectangle {
        x: control.width - width - 3
        y: 3
        implicitWidth: 26
        height: control.height - 6
        radius: Theme.radiusSm
        color: control.up.pressed ? Theme.surfacePressed
                                  : (control.up.hovered && enabled ? Theme.surfaceHover : Theme.alpha(Theme.surfaceHover, 0))
        Icon {
            anchors.centerIn: parent
            name: "plus"
            size: 14
            color: control.up.hovered ? Theme.text : Theme.textMuted
            opacity: parent.enabled ? 1 : 0.35
        }
    }

    background: Rectangle {
        implicitWidth: 112
        radius: Theme.radiusMd
        color: Theme.inputBg
        border.width: 1
        border.color: control.activeFocus ? Theme.accent : (control.hovered ? Theme.borderStrong : Theme.border)
        Behavior on border.color { ColorAnimation { duration: Theme.fast } }

        Rectangle {
            anchors.fill: parent
            anchors.margins: -3
            radius: parent.radius + 3
            color: "transparent"
            border.width: 3
            border.color: Theme.alpha(Theme.accent, 0.2)
            visible: control.activeFocus
        }
    }
}
