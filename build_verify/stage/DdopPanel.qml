import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

GroupBox {
    title: "Device descriptor (DDOP)"
    ColumnLayout {
        anchors.fill: parent
        spacing: 4

        TabBar {
            id: tabs
            Layout.fillWidth: true
            TabButton { text: "Objects" }
            TabButton { text: "Declared DDIs (" + bridge.implementDdis.length + ")" }
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: tabs.currentIndex

            ListView {
                id: ddopList
                model: ddopModel
                clip: true
                delegate: Text {
                    width: ddopList.width
                    leftPadding: 6 + indent * 18
                    topPadding: 2
                    bottomPadding: 2
                    color: indent === 0 ? "#7fd0ff" : "#dfe6ee"
                    font.pixelSize: indent === 0 ? 13 : 12
                    font.bold: indent === 0
                    wrapMode: Text.Wrap
                    text: rowText
                }
            }

            ColumnLayout {
                spacing: 4
                RowLayout {
                    Layout.fillWidth: true
                    CheckBox {
                        text: "Automatic synchronization"
                        checked: bridge.autoDdiSync
                        onToggled: bridge.setAutoDdiSync(checked)
                    }
                    Label { text: "Interval"; color: "#9fb0c3" }
                    SpinBox {
                        from: 250
                        to: 60000
                        stepSize: 250
                        value: bridge.ddiSyncIntervalMs
                        editable: true
                        Layout.preferredWidth: 100
                        onValueModified: bridge.setDdiSyncIntervalMs(value)
                    }
                    Button { text: "Request all"; onClicked: bridge.requestImplementDdis() }
                }
                Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: "#3a4048" }
                ListView {
                    id: ddiList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    model: bridge.implementDdis
                    clip: true
                    delegate: Rectangle {
                        width: ddiList.width
                        height: 42
                        color: index % 2 ? "#151a20" : "transparent"
                        Column {
                            anchors.fill: parent
                            anchors.leftMargin: 6
                            Text {
                                text: "DDI " + modelData.ddi + "  element " + modelData.element + "  " + modelData.name
                                color: "#dfe6ee"
                                font.pixelSize: 12
                            }
                            Text {
                                text: (modelData.hasValue ? "Value " + modelData.value + "  " + modelData.updated : "Waiting for value")
                                      + "  •  " + modelData.triggers + (modelData.settable ? "  •  settable" : "")
                                color: modelData.hasValue ? "#8fe388" : "#9fb0c3"
                                font.pixelSize: 11
                            }
                        }
                    }
                }
            }
        }
    }
}
