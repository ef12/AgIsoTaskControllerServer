import QtQuick
import QtQuick.Templates as T
import AgIsoTc 1.0

// Tooltip of the design system. Shown above its parent, or below it when there is no room above.
T.ToolTip {
    id: control

    x: parent ? (parent.width - width) / 2 : 0
    y: {
        if (!parent)
            return 0
        const top = parent.mapToItem(null, 0, 0).y
        return top < implicitHeight + 16 ? parent.height + 6 : -implicitHeight - 6
    }

    implicitWidth: Math.min(320, Math.max(implicitBackgroundWidth + leftInset + rightInset,
                                          implicitContentWidth + leftPadding + rightPadding))
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    margins: 8
    topPadding: 6
    bottomPadding: 6
    leftPadding: 10
    rightPadding: 10
    delay: 450
    timeout: 8000
    closePolicy: T.Popup.CloseOnEscape | T.Popup.CloseOnPressOutsideParent | T.Popup.CloseOnReleaseOutsideParent

    font.family: Theme.fontFamily
    font.pixelSize: Theme.fontSmall

    contentItem: Text {
        text: control.text
        font: control.font
        color: Theme.tooltipText
        wrapMode: Text.Wrap
        lineHeight: 1.1
    }

    background: Rectangle {
        color: Theme.tooltipBg
        radius: Theme.radiusSm
        border.color: Qt.rgba(1, 1, 1, 0.06)
    }

    enter: Transition {
        NumberAnimation { property: "opacity"; from: 0; to: 1; duration: Theme.fast }
    }
    exit: Transition {
        NumberAnimation { property: "opacity"; from: 1; to: 0; duration: Theme.fast }
    }
}
