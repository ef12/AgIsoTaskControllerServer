import QtQuick
import AgIsoTc 1.0

// A row of tabs with a sliding marker.
//   variant "segmented": a pill with a raised segment under the current tab
//   variant "underline": plain text tabs with an accent bar under the current tab
//   model:   strings, or objects { text, count, icon }
//   fill:    tabs share the whole width
Item {
    id: root

    property var model: []
    property int currentIndex: 0
    property string variant: "segmented"
    property bool fill: false

    signal activated(int index)

    readonly property bool segmented: variant === "segmented"
    readonly property int inset: segmented ? 3 : 0
    property bool _animate: false

    implicitHeight: segmented ? 32 : 40
    implicitWidth: row.implicitWidth + 2 * inset

    function place() {
        const item = repeater.itemAt(currentIndex)
        if (!item) {
            marker.width = 0
            return
        }
        const indent = segmented ? 0 : 10
        marker.x = row.x + item.x + indent
        marker.width = item.width - 2 * indent
    }

    onCurrentIndexChanged: place()
    onWidthChanged: place()
    Component.onCompleted: Qt.callLater(function() { root.place(); root._animate = true })

    Rectangle {
        anchors.fill: parent
        visible: root.segmented
        radius: Theme.radiusMd
        color: Theme.inputBg
        border.color: Theme.border
    }

    Rectangle {
        id: marker
        visible: width > 0
        y: root.segmented ? root.inset : root.height - 2
        height: root.segmented ? root.height - 2 * root.inset : 2
        radius: root.segmented ? Theme.radiusSm : 1
        color: root.segmented ? Theme.surfaceHover : Theme.accent
        border.width: root.segmented ? 1 : 0
        border.color: Theme.borderStrong
        Behavior on x { enabled: root._animate; NumberAnimation { duration: Theme.normal; easing.type: Easing.OutCubic } }
        Behavior on width { enabled: root._animate; NumberAnimation { duration: Theme.normal; easing.type: Easing.OutCubic } }
    }

    Row {
        id: row
        x: root.inset
        y: root.inset
        height: root.height - 2 * root.inset
        spacing: root.segmented ? 0 : 2

        Repeater {
            id: repeater
            model: root.model
            onItemAdded: root.place()

            delegate: Item {
                id: tab

                readonly property var entry: typeof modelData === "string" ? { "text": modelData } : modelData
                readonly property bool current: index === root.currentIndex

                width: root.fill ? (root.width - 2 * root.inset - row.spacing * (repeater.count - 1)) / Math.max(1, repeater.count)
                                 : content.implicitWidth + (root.segmented ? 26 : 20)
                height: row.height
                onWidthChanged: if (current) root.place()
                onXChanged: if (current) root.place()

                Row {
                    id: content
                    anchors.centerIn: parent
                    spacing: 6

                    Icon {
                        visible: !!tab.entry.icon
                        anchors.verticalCenter: parent.verticalCenter
                        name: tab.entry.icon || ""
                        size: 14
                        color: label.color
                    }
                    Text {
                        id: label
                        anchors.verticalCenter: parent.verticalCenter
                        text: tab.entry.text
                        font.family: Theme.fontFamily
                        font.pixelSize: root.segmented ? Theme.fontSmall : Theme.fontBody
                        font.weight: Font.DemiBold
                        color: tab.current ? Theme.text : (mouse.containsMouse ? Theme.text : Theme.textMuted)
                        Behavior on color { ColorAnimation { duration: Theme.fast } }
                    }
                    Rectangle {
                        visible: tab.entry.count !== undefined && tab.entry.count !== null
                        anchors.verticalCenter: parent.verticalCenter
                        implicitWidth: Math.max(18, countText.implicitWidth + 10)
                        implicitHeight: 18
                        radius: 9
                        color: tab.current ? Theme.alpha(Theme.accent, 0.18) : Theme.surfacePressed
                        Text {
                            id: countText
                            anchors.centerIn: parent
                            text: tab.entry.count !== undefined ? tab.entry.count : ""
                            font.family: Theme.fontFamily
                            font.pixelSize: Theme.fontCaption
                            font.weight: Font.DemiBold
                            color: tab.current ? Theme.accentText : Theme.textMuted
                        }
                    }
                }

                MouseArea {
                    id: mouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        root.currentIndex = index
                        root.activated(index)
                    }
                }
            }
        }
    }
}
