import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

GroupBox {
    title: "Commands (selected client)"
    ScrollView {
        anchors.fill: parent
        clip: true
        ColumnLayout {
            width: parent.width
            spacing: 6
            Label {
                text: "Target client: " + (bridge.selectedClient >= 0 ? bridge.selectedClient : "none")
                color: "#c7d0dc"
                font.bold: true
            }
            RowLayout {
                Label {
                    text: "DDI:"
                    color: "#c7d0dc"
                }
                SpinBox {
                    id: ddiSpin
                    from: 0
                    to: 65535
                    value: 0
                    editable: true
                    Layout.fillWidth: true
                }
                Label {
                    text: "Elem:"
                    color: "#c7d0dc"
                }
                SpinBox {
                    id: elemSpin
                    from: 0
                    to: 65535
                    value: 1
                    editable: true
                    Layout.fillWidth: true
                }
            }
            RowLayout {
                Label {
                    text: "Value:"
                    color: "#c7d0dc"
                }
                SpinBox {
                    id: valueSpin
                    from: -2147483647
                    to: 2147483647
                    value: 0
                    editable: true
                    Layout.fillWidth: true
                }
            }
            RowLayout {
                Button {
                    text: "Request"
                    Layout.fillWidth: true
                    onClicked: bridge.requestValue(ddiSpin.value, elemSpin.value)
                }
                Button {
                    text: "Set"
                    Layout.fillWidth: true
                    onClicked: bridge.setValue(ddiSpin.value, elemSpin.value, valueSpin.value, false)
                }
                Button {
                    text: "Set+Ack"
                    Layout.fillWidth: true
                    onClicked: bridge.setValue(ddiSpin.value, elemSpin.value, valueSpin.value, true)
                }
            }
            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: "#3a4048"
            }
            Label {
                text: "Measurement trigger"
                color: "#c7d0dc"
                font.bold: true
            }
            ComboBox {
                id: measureBox
                Layout.fillWidth: true
                model: ["Time interval (ms)", "Distance interval (mm)", "Minimum threshold", "Maximum threshold", "Change threshold"]
            }
            RowLayout {
                Label {
                    text: "Trigger:"
                    color: "#c7d0dc"
                }
                SpinBox {
                    id: triggerSpin
                    from: 0
                    to: 2147483647
                    value: 1000
                    editable: true
                    Layout.fillWidth: true
                }
            }
            Button {
                text: "Send measurement command"
                Layout.fillWidth: true
                onClicked: bridge.sendMeasurement(measureBox.currentIndex + 4, ddiSpin.value, elemSpin.value, triggerSpin.value)
            }
        }
    }
}
