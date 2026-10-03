import QtQuick
import QtQuick.Templates as T
import AgIsoTc 1.0

// Single-line text input. The frame turns red while a validator rejects the text.
T.TextField {
    id: control

    readonly property bool invalid: !acceptableInput && length > 0

    implicitWidth: implicitBackgroundWidth + leftInset + rightInset
                   || Math.max(contentWidth, placeholder.implicitWidth) + leftPadding + rightPadding
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             contentHeight + topPadding + bottomPadding,
                             placeholder.implicitHeight + topPadding + bottomPadding)

    leftPadding: 10
    rightPadding: 10
    topPadding: 6
    bottomPadding: 6
    verticalAlignment: TextInput.AlignVCenter
    selectByMouse: true
    hoverEnabled: true
    opacity: enabled ? 1 : 0.5

    color: Theme.text
    selectionColor: Theme.selection
    selectedTextColor: Theme.text
    placeholderTextColor: Theme.textMuted
    font.family: Theme.fontFamily
    font.pixelSize: Theme.fontBody

    Text {
        id: placeholder
        x: control.leftPadding
        y: control.topPadding
        width: control.width - control.leftPadding - control.rightPadding
        height: control.height - control.topPadding - control.bottomPadding
        text: control.placeholderText
        font: control.font
        color: control.placeholderTextColor
        verticalAlignment: control.verticalAlignment
        elide: Text.ElideRight
        visible: !control.length && !control.preeditText
    }

    background: Rectangle {
        implicitWidth: 160
        implicitHeight: Theme.controlHeight
        radius: Theme.radiusMd
        color: Theme.inputBg
        border.width: 1
        border.color: control.invalid ? Theme.danger
                                      : (control.activeFocus ? Theme.accent
                                                             : (control.hovered ? Theme.borderStrong : Theme.border))
        Behavior on border.color { ColorAnimation { duration: Theme.fast } }

        Rectangle {
            anchors.fill: parent
            anchors.margins: -3
            radius: parent.radius + 3
            color: "transparent"
            border.width: 3
            border.color: Theme.alpha(control.invalid ? Theme.danger : Theme.accent, 0.2)
            visible: control.activeFocus
        }
    }
}
