import QtQuick
import QtQuick.Effects
import QtQuick.Templates as T
import AgIsoTc 1.0

// Drop-down list. The list opens as a floating card; the current entry carries a check mark.
T.ComboBox {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Theme.controlHeight

    leftPadding: 10
    rightPadding: 32
    hoverEnabled: true
    opacity: enabled ? 1 : 0.5

    font.family: Theme.fontFamily
    font.pixelSize: Theme.fontBody

    delegate: T.ItemDelegate {
        id: entry

        readonly property bool current: control.currentIndex === index

        width: ListView.view ? ListView.view.width : 0
        implicitHeight: 30
        leftPadding: 10
        rightPadding: 10
        highlighted: control.highlightedIndex === index
        hoverEnabled: true

        contentItem: Item {
            Text {
                anchors.left: parent.left
                anchors.right: check.left
                anchors.rightMargin: 8
                anchors.verticalCenter: parent.verticalCenter
                text: control.textAt(index)
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontBody
                font.weight: entry.current ? Font.DemiBold : Font.Normal
                color: entry.current ? Theme.accentText : Theme.text
                elide: Text.ElideRight
            }
            Icon {
                id: check
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                name: "check"
                size: 14
                color: Theme.accentText
                visible: entry.current
            }
        }

        background: Rectangle {
            radius: Theme.radiusSm
            color: entry.highlighted || entry.hovered ? Theme.surfaceHover : Theme.alpha(Theme.surfaceHover, 0)
        }
    }

    indicator: Icon {
        x: control.width - width - 10
        y: (control.height - height) / 2
        name: "chevronDown"
        size: 16
        color: control.hovered ? Theme.textSecondary : Theme.textMuted
        rotation: control.popup.visible ? 180 : 0
        Behavior on rotation { NumberAnimation { duration: Theme.normal; easing.type: Easing.OutCubic } }
    }

    contentItem: Text {
        text: control.displayText
        font: control.font
        color: Theme.text
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    background: Rectangle {
        implicitWidth: 140
        radius: Theme.radiusMd
        color: control.down ? Theme.surfacePressed : Theme.inputBg
        border.width: 1
        border.color: control.popup.visible || control.visualFocus ? Theme.accent
                                                                   : (control.hovered ? Theme.borderStrong : Theme.border)
        Behavior on border.color { ColorAnimation { duration: Theme.fast } }
    }

    popup: T.Popup {
        y: control.height + 4
        width: control.width
        height: Math.min(contentItem.implicitHeight + topPadding + bottomPadding,
                         control.Window.height - topMargin - bottomMargin)
        topMargin: 8
        bottomMargin: 8
        padding: 4

        contentItem: ListView {
            clip: true
            implicitHeight: contentHeight
            model: control.delegateModel
            currentIndex: control.highlightedIndex
            highlightMoveDuration: 0
            boundsBehavior: Flickable.StopAtBounds
            T.ScrollBar.vertical: AppScrollBar { }
        }

        background: Rectangle {
            radius: Theme.radiusMd
            color: Theme.popupBg
            border.color: Theme.borderStrong
            layer.enabled: true
            layer.effect: MultiEffect {
                shadowEnabled: true
                shadowColor: Theme.shadow
                shadowBlur: 0.7
                shadowVerticalOffset: 6
            }
        }

        enter: Transition {
            NumberAnimation { property: "opacity"; from: 0; to: 1; duration: Theme.fast }
            NumberAnimation { property: "scale"; from: 0.97; to: 1; duration: Theme.normal; easing.type: Easing.OutCubic }
        }
        exit: Transition {
            NumberAnimation { property: "opacity"; from: 1; to: 0; duration: Theme.fast }
        }
    }
}
