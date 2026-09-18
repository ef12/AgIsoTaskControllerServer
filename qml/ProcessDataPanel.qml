import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

GroupBox {
    title: "Live process data"
    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        Row {
            Layout.fillWidth: true
            height: 22
            Text {
                width: 60
                text: "Addr"
                color: "#7fd0ff"
                font.bold: true
                font.pixelSize: 12
            }
            Text {
                width: 90
                text: "DDI"
                color: "#7fd0ff"
                font.bold: true
                font.pixelSize: 12
            }
            Text {
                width: 90
                text: "Element"
                color: "#7fd0ff"
                font.bold: true
                font.pixelSize: 12
            }
            Text {
                width: 140
                text: "Value"
                color: "#7fd0ff"
                font.bold: true
                font.pixelSize: 12
            }
            Text {
                text: "Time"
                color: "#7fd0ff"
                font.bold: true
                font.pixelSize: 12
            }
        }
        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#3a4048"
        }
        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: valueModel
            clip: true
            delegate: Row {
                width: parent ? parent.width : 400
                height: 22
                Text {
                    width: 60
                    text: valueAddress
                    color: "#dfe6ee"
                    font.pixelSize: 12
                }
                Text {
                    width: 90
                    text: valueDdi
                    color: "#dfe6ee"
                    font.pixelSize: 12
                }
                Text {
                    width: 90
                    text: valueElement
                    color: "#dfe6ee"
                    font.pixelSize: 12
                }
                Text {
                    width: 140
                    text: valueContent
                    color: "#8fe388"
                    font.pixelSize: 12
                }
                Text {
                    text: valueTime
                    color: "#9fb0c3"
                    font.pixelSize: 12
                }
            }
        }
    }
}
