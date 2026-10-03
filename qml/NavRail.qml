import QtQuick
import QtQuick.Layouts
import AgIsoTc 1.0
import "components"

// The navigation rail on the left: one entry per side panel page, each with an optional count
// or status dot. Clicking the open page again closes the side panel. The theme switch sits at
// the bottom.
Item {
    id: root

    property var pages: []
    property var indicators: []      // per page: { count } or { tone }
    property int currentIndex: 0
    property bool panelOpen: true

    signal activated(int index)

    implicitWidth: 76

    ColumnLayout {
        anchors.fill: parent
        anchors.topMargin: 2
        anchors.bottomMargin: 12
        spacing: 2

        Repeater {
            model: root.pages

            delegate: Item {
                id: entry

                readonly property bool current: index === root.currentIndex && root.panelOpen
                readonly property var indicator: root.indicators[index] || ({})

                Layout.alignment: Qt.AlignHCenter
                implicitWidth: 68
                implicitHeight: 60

                Rectangle {
                    id: pill
                    anchors.horizontalCenter: parent.horizontalCenter
                    y: 6
                    width: 48
                    height: 30
                    radius: 15
                    color: entry.current ? Theme.alpha(Theme.accent, Theme.dark ? 0.17 : 0.13)
                                         : (mouse.containsMouse ? Theme.surfaceHover : Theme.alpha(Theme.surfaceHover, 0))
                    Behavior on color { ColorAnimation { duration: Theme.fast } }

                    Icon {
                        anchors.centerIn: parent
                        name: modelData.icon
                        size: 19
                        color: entry.current ? Theme.accentText : (mouse.containsMouse ? Theme.text : Theme.textSecondary)
                    }

                    // count badge
                    Rectangle {
                        visible: (entry.indicator.count || 0) > 0
                        x: parent.width - width + 2
                        y: -3
                        implicitWidth: Math.max(16, countText.implicitWidth + 8)
                        implicitHeight: 16
                        radius: 8
                        color: Theme.accent
                        border.width: 2
                        border.color: Theme.bg
                        Text {
                            id: countText
                            anchors.centerIn: parent
                            text: entry.indicator.count || ""
                            font.family: Theme.fontFamily
                            font.pixelSize: 10
                            font.weight: Font.Bold
                            color: Theme.onAccent
                        }
                    }

                    // status dot
                    Rectangle {
                        visible: !!entry.indicator.tone
                        x: parent.width - width - 6
                        y: 3
                        width: 9
                        height: 9
                        radius: 4.5
                        color: Theme.tone(entry.indicator.tone || "neutral")
                        border.width: 2
                        border.color: Theme.bg
                    }
                }

                Text {
                    anchors.top: pill.bottom
                    anchors.topMargin: 4
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: modelData.label
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontCaption
                    font.weight: entry.current ? Font.DemiBold : Font.Normal
                    color: entry.current ? Theme.text : Theme.textMuted
                }

                MouseArea {
                    id: mouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.activated(index)
                }
            }
        }

        Item { Layout.fillHeight: true }

        AppButton {
            Layout.alignment: Qt.AlignHCenter
            variant: "ghost"
            iconName: Theme.dark ? "sun" : "moon"
            tip: Theme.dark ? "Switch to the light theme" : "Switch to the dark theme"
            onClicked: Theme.dark = !Theme.dark
        }
    }
}
