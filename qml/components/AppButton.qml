import QtQuick
import QtQuick.Templates as T
import AgIsoTc 1.0

// Push button of the design system.
//   variant:  "primary" (accent fill), "secondary" (framed surface), "ghost" (no frame until
//             hovered), "danger" (red fill) or "dangerSoft" (red tint)
//   size:     "sm", "md" or "lg"
//   iconName: an Icons.js icon before the text; without text the button is a square icon button
//   tip:      tooltip text
// A checkable button shows its checked state with the accent tint; `active` gives the same
// look to a plain button that reflects a state it does not own (e.g. following the tractor).
T.Button {
    id: control

    property string variant: "secondary"
    property string size: "md"
    property string iconName: ""
    property string tip: ""
    property bool active: false
    property real iconSize: size === "sm" ? 14 : (size === "lg" ? 18 : 16)

    readonly property int controlHeight: size === "sm" ? Theme.controlHeightSm
                                                       : (size === "lg" ? Theme.controlHeightLg : Theme.controlHeight)
    readonly property bool iconOnly: text === "" && iconName !== ""

    readonly property color foreground: {
        if (checked || active)
            return Theme.accentText
        switch (variant) {
        case "primary": return Theme.onAccent
        case "danger": return "#ffffff"
        case "dangerSoft": return Theme.danger
        case "ghost": return hovered ? Theme.text : Theme.textSecondary
        default: return Theme.text
        }
    }
    readonly property color fill: {
        if (checked || active)
            return Theme.alpha(Theme.accent, down ? 0.28 : (hovered ? 0.22 : 0.15))
        switch (variant) {
        case "primary": return down ? Theme.accentPressed : (hovered ? Theme.accentHover : Theme.accent)
        case "danger": return down ? Qt.darker(Theme.danger, 1.15) : (hovered ? Qt.lighter(Theme.danger, 1.08) : Theme.danger)
        case "dangerSoft": return Theme.alpha(Theme.danger, down ? 0.28 : (hovered ? 0.2 : 0.13))
        case "ghost": return down ? Theme.surfacePressed : (hovered ? Theme.surfaceHover : Theme.alpha(Theme.surfaceHover, 0))
        default: return down ? Theme.surfacePressed : (hovered ? Theme.surfaceHover : Theme.surfaceAlt)
        }
    }
    readonly property color frame: {
        if (checked || active)
            return Theme.alpha(Theme.accent, 0.45)
        switch (variant) {
        case "secondary": return hovered ? Theme.borderStrong : Theme.border
        case "dangerSoft": return Theme.alpha(Theme.danger, 0.35)
        default: return Theme.alpha(Theme.border, 0)
        }
    }

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: controlHeight

    padding: 0
    leftPadding: iconOnly ? 0 : (size === "sm" ? 10 : 14)
    rightPadding: leftPadding
    spacing: 7
    hoverEnabled: true
    opacity: enabled ? 1 : 0.4
    Accessible.name: text !== "" ? text : tip

    font.family: Theme.fontFamily
    font.pixelSize: size === "sm" ? Theme.fontSmall : Theme.fontBody
    font.weight: Font.DemiBold

    contentItem: Item {
        implicitWidth: row.implicitWidth
        implicitHeight: row.implicitHeight

        Row {
            id: row
            anchors.centerIn: parent
            spacing: control.spacing

            Icon {
                visible: control.iconName !== ""
                anchors.verticalCenter: parent.verticalCenter
                name: control.iconName
                size: control.iconSize
                color: control.foreground
            }
            Text {
                visible: control.text !== ""
                anchors.verticalCenter: parent.verticalCenter
                text: control.text
                font: control.font
                color: control.foreground
            }
        }
    }

    background: Rectangle {
        implicitWidth: control.controlHeight
        implicitHeight: control.controlHeight
        radius: Theme.radiusMd
        color: control.fill
        border.width: 1
        border.color: control.frame
        Behavior on color { ColorAnimation { duration: Theme.fast } }

        // keyboard focus ring
        Rectangle {
            anchors.fill: parent
            anchors.margins: -3
            radius: parent.radius + 3
            color: "transparent"
            border.width: 2
            border.color: Theme.alpha(Theme.accent, 0.55)
            visible: control.visualFocus
        }
    }

    AppToolTip {
        text: control.tip
        visible: control.tip !== "" && control.hovered
    }
}
