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
            TabButton { text: "DDI traffic" }
            TabButton { text: "Event log" }
        }
        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: tabs.currentIndex
            ColumnLayout {
                Label {
                    Layout.fillWidth: true
                    visible: bridge.tcBasicData.length === 0
                    text: "No implement data yet. Select the client (its pool loads automatically), then start a task — rows appear as values arrive."
                    color: "#8995a3"
                    font.pixelSize: 12
                    wrapMode: Text.Wrap
                }
                ListView {
                    id: basicList
                    visible: bridge.tcBasicData.length > 0
                    Layout.fillWidth: true
                    Layout.fillHeight: true
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
                RowLayout {
                    Layout.fillWidth: true
                    CheckBox {
                        text: "Automatic section control"
                        checked: bridge.autoSectionControl
                        onToggled: bridge.setAutoSectionControl(checked)
                        ToolTip.visible: hovered
                        ToolTip.text: "While a task is active, the TC switches the client to automatic and turns its sections on and off from the field boundary and the coverage."
                    }
                    Label {
                        text: bridge.sectionControlStatus
                        color: bridge.sectionControlStatus.startsWith("Automatic") ? "#8fe388" : "#9fb0c3"
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                    }
                }
                Label {
                    text: bridge.activeSectionCount + " of " + bridge.sectionCount + " sections ON (as the client reports)"
                    color: bridge.activeSectionCount > 0 ? "#f2d33c" : "#8995a3"
                    font.bold: true
                }
                Label {
                    visible: bridge.rateSetpoints.length === 0
                    text: "Rate control: the client's DDOP offers no rate setpoints."
                    color: "#8995a3"
                    font.pixelSize: 12
                }
                Repeater {
                    model: bridge.rateSetpoints
                    delegate: RowLayout {
                        Layout.fillWidth: true
                        Label {
                            text: "Rate " + (modelData.name !== "" ? modelData.name : "setpoint") + " (DDI " + modelData.ddi + ", element " + modelData.element + ")"
                            color: "#dfe6ee"
                            Layout.fillWidth: true
                        }
                        SpinBox {
                            from: 0
                            to: 2000000000
                            editable: true
                            value: modelData.target
                            onValueModified: bridge.setRateTarget(index, value)
                            ToolTip.visible: hovered
                            ToolTip.text: "Rate the TC commands while a task is active, in the DDOP's raw unit. 0 sends nothing."
                        }
                    }
                }
                // One LED bar per boom, as wide as the boom, an LED per section.
                BoomLedBars {
                    visible: bridge.booms.length > 0
                    Layout.fillWidth: true
                }
                Item {
                    visible: bridge.booms.length > 0
                    Layout.fillHeight: true
                }
                // Without a client DDOP: the sections of the manual section DDI, one per button.
                Flow {
                    visible: bridge.booms.length === 0
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 5
                    Repeater {
                        model: bridge.sectionStates
                        delegate: Rectangle {
                            width: 46
                            height: 30
                            radius: 4
                            color: modelData ? "#b59d19" : "#29313a"
                            border.color: modelData ? "#f2d33c" : "#59636f"
                            Label {
                                anchors.centerIn: parent
                                text: (index + 1) + " " + (modelData ? "ON" : "OFF")
                                color: modelData ? "white" : "#9fb0c3"
                                font.pixelSize: 11
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
            ColumnLayout {
                spacing: 4
                RowLayout {
                    Layout.fillWidth: true
                    CheckBox {
                        text: "Live watch"
                        checked: bridge.liveDdiTrafficWatch
                        onToggled: bridge.setLiveDdiTrafficWatch(checked)
                    }
                    Label {
                        text: bridge.liveDdiTrafficWatch ? "showing continuous TC/client DDI traffic" : "off"
                        color: bridge.liveDdiTrafficWatch ? "#8fe388" : "#8995a3"
                    }
                    Item { Layout.fillWidth: true }
                    Button {
                        text: "Clear"
                        onClicked: bridge.clearDdiTraffic()
                    }
                }
                Row {
                    Layout.fillWidth: true
                    height: 22
                    Text { width: 80; text: "Time"; color: "#7fd0ff"; font.bold: true; font.pixelSize: 12 }
                    Text { width: 92; text: "Direction"; color: "#7fd0ff"; font.bold: true; font.pixelSize: 12 }
                    Text { width: 104; text: "Command"; color: "#7fd0ff"; font.bold: true; font.pixelSize: 12 }
                    Text { width: 54; text: "Addr"; color: "#7fd0ff"; font.bold: true; font.pixelSize: 12 }
                    Text { width: 74; text: "DDI"; color: "#7fd0ff"; font.bold: true; font.pixelSize: 12 }
                    Text { width: 70; text: "Elem"; color: "#7fd0ff"; font.bold: true; font.pixelSize: 12 }
                    Text { width: 88; text: "Value"; color: "#7fd0ff"; font.bold: true; font.pixelSize: 12 }
                    Text { text: "Detail"; color: "#7fd0ff"; font.bold: true; font.pixelSize: 12 }
                }
                Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: "#3a4048" }
                ListView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    model: ddiTrafficModel
                    clip: true
                    delegate: Rectangle {
                        width: ListView.view ? ListView.view.width : 600
                        height: 22
                        color: index % 2 ? "#151a20" : "transparent"
                        Row {
                            anchors.fill: parent
                            Text { width: 80; text: trafficTime; color: "#9fb0c3"; font.pixelSize: 12 }
                            Text { width: 92; text: trafficDirection; color: trafficDirection.indexOf("TC") === 0 ? "#73c7ff" : "#f2d33c"; font.pixelSize: 12 }
                            Text { width: 104; text: trafficCommand; color: "#dfe6ee"; font.pixelSize: 12; elide: Text.ElideRight }
                            Text { width: 54; text: trafficAddress; color: "#dfe6ee"; font.pixelSize: 12 }
                            Text { width: 74; text: trafficDdi; color: "#dfe6ee"; font.pixelSize: 12 }
                            Text { width: 70; text: trafficElement; color: "#dfe6ee"; font.pixelSize: 12 }
                            Text { width: 88; text: trafficValue; color: "#8fe388"; font.pixelSize: 12 }
                            Text { text: trafficDetail; color: "#8995a3"; font.pixelSize: 12; elide: Text.ElideRight }
                        }
                    }
                }
            }
            // Event log: what the server did and saw, newest at the bottom.
            ColumnLayout {
                spacing: 4
                ListView {
                    id: logList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    model: logModel
                    clip: true
                    onCountChanged: logList.positionViewAtEnd()
                    delegate: Text {
                        width: logList.width
                        color: "#b9c4d2"
                        font.family: "Consolas, monospace"
                        font.pixelSize: 11
                        wrapMode: Text.Wrap
                        text: logLine
                    }
                }
                RowLayout {
                    Layout.fillWidth: true
                    Item { Layout.fillWidth: true }
                    Button {
                        text: "Clear"
                        onClicked: bridge.clearLog()
                    }
                }
            }
        }
    }
}
