import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

GroupBox {
    title: "Task Controller data"
    ColumnLayout {
        anchors.fill: parent
        spacing: 4
        TabBar {
            id: tabs
            Layout.fillWidth: true
            TabButton { text: "TC-Basic" }
            TabButton { text: "TC-SC" }
            TabButton { text: "Raw process data" }
        }
        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: tabs.currentIndex
            ListView {
                id: basicList
                model: bridge.tcBasicData
                clip: true
                delegate: Rectangle {
                    width: basicList.width
                    height: 32
                    color: index % 2 ? "#151a20" : "transparent"
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 8
                        anchors.rightMargin: 8
                        Label { text: modelData.label; color: "#dfe6ee"; Layout.fillWidth: true }
                        Label { text: "DDI " + modelData.ddi + " / " + modelData.element; color: "#8995a3" }
                        Label {
                            text: modelData.hasValue ? Number(modelData.displayValue).toLocaleString(Qt.locale(), 'f', 2) + " " + modelData.unit : "waiting"
                            color: modelData.hasValue ? "#8fe388" : "#8995a3"
                            font.bold: modelData.hasValue
                            Layout.preferredWidth: 150
                            horizontalAlignment: Text.AlignRight
                        }
                    }
                }
            }
            ColumnLayout {
                spacing: 7
                RowLayout {
                    Layout.fillWidth: true
                    Label { text: "Worked area"; color: "#9fb0c3" }
                    Label { text: bridge.workedAreaHa.toFixed(3) + " ha"; color: "#8fe388"; font.bold: true }
                    Label { text: "Distance"; color: "#9fb0c3" }
                    Label { text: bridge.workedDistanceM.toFixed(1) + " m"; color: "#dfe6ee" }
                    Label { text: "Time"; color: "#9fb0c3" }
                    Label { text: Math.floor(bridge.workedTimeSeconds / 60) + "m " + Math.floor(bridge.workedTimeSeconds % 60) + "s"; color: "#dfe6ee" }
                    Item { Layout.fillWidth: true }
                    Button { text: "Clear coverage"; onClicked: bridge.clearWorkedArea() }
                }
                Label {
                    text: bridge.activeSectionCount + " of " + bridge.sectionCount + " sections ON"
                    color: bridge.activeSectionCount > 0 ? "#f2d33c" : "#8995a3"
                    font.bold: true
                }
                Flow {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 5
                    Repeater {
                        model: bridge.sectionStates
                        delegate: Rectangle {
                            width: 58
                            height: 38
                            radius: 4
                            color: modelData ? "#b59d19" : "#29313a"
                            border.color: modelData ? "#f2d33c" : "#59636f"
                            Label {
                                anchors.centerIn: parent
                                text: (index + 1) + "  " + (modelData ? "ON" : "OFF")
                                color: modelData ? "white" : "#9fb0c3"
                                font.bold: modelData
                            }
                        }
                    }
                }
            }
            ColumnLayout {
                spacing: 0
                Row {
                    Layout.fillWidth: true
                    height: 22
                    Text { width: 60; text: "Addr"; color: "#7fd0ff"; font.bold: true; font.pixelSize: 12 }
                    Text { width: 90; text: "DDI"; color: "#7fd0ff"; font.bold: true; font.pixelSize: 12 }
                    Text { width: 90; text: "Element"; color: "#7fd0ff"; font.bold: true; font.pixelSize: 12 }
                    Text { width: 140; text: "Value"; color: "#7fd0ff"; font.bold: true; font.pixelSize: 12 }
                    Text { text: "Time"; color: "#7fd0ff"; font.bold: true; font.pixelSize: 12 }
                }
                Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: "#3a4048" }
                ListView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    model: valueModel
                    clip: true
                    delegate: Row {
                        width: parent ? parent.width : 400
                        height: 22
                        Text { width: 60; text: valueAddress; color: "#dfe6ee"; font.pixelSize: 12 }
                        Text { width: 90; text: valueDdi; color: "#dfe6ee"; font.pixelSize: 12 }
                        Text { width: 90; text: valueElement; color: "#dfe6ee"; font.pixelSize: 12 }
                        Text { width: 140; text: valueContent; color: "#8fe388"; font.pixelSize: 12 }
                        Text { text: valueTime; color: "#9fb0c3"; font.pixelSize: 12 }
                    }
                }
            }
        }
    }
}
