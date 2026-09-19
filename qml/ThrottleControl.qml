import QtQuick
import QtQuick.Controls

Item {
    id: root
    property double value: 0.0 // 0.0 to 1.0
    property bool enabled: true

    signal valueChanged(double value)
    signal stopRequested()

    width: 120
    height: 320

    // Throttle track
    Rectangle {
        id: track
        width: 60
        height: parent.height - 100
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 20
        radius: width/2
        color: "#263238"
        border.color: "#455A64"
        border.width: 2

        // Fill level
        Rectangle {
            id: fill
            width: parent.width
            height: parent.height * root.value
            anchors.bottom: parent.bottom
            radius: parent.radius
            color: root.value > 0.7 ? "#F44336" : (root.value > 0.3 ? "#FF9800" : "#4CAF50")
            behavior: NumberAnimation { duration: 200 }
        }

        // Tick marks
        Repeater {
            model: 11
            delegate: Item {
                width: parent.width
                height: parent.height / 10
                Label {
                    anchors.right: parent.right
                    anchors.rightMargin: 8
                    anchors.verticalCenter: parent.verticalCenter
                    text: (10 - index) * 10 + "%"
                    color: "#90A4AE"
                    font.pixelSize: 11
                }
            }
        }

        // Current value indicator
        Rectangle {
            width: parent.width + 10
            height: 4
            y: parent.height * (1 - root.value) - 2
            color: "#FFEB3B"
            border.color: "#FDD835"
            border.width: 1
            radius: 2
        }
    }

    // Control buttons
    Column {
        anchors.top: track.bottom
        anchors.topMargin: 16
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: 12

        // Plus button
        Button {
            id: plusBtn
            width: 70
            height: 50
            enabled: root.enabled && root.value < 1.0
            contentItem: Text {
                text: "+"
                font.pixelSize: 28
                font.bold: true
                color: "#FFFFFF"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            background: Rectangle {
                radius: 8
                color: plusBtn.pressed ? "#388E3C" : plusBtn.enabled ? "#4CAF50" : "#81C784"
                border.color: "#2E7D32"
                border.width: plusBtn.pressed ? 0 : 2
            }
            onClicked: {
                var newVal = Math.min(1.0, root.value + 0.1);
                root.value = newVal;
                root.valueChanged(newVal);
            }
            autoRepeat: true
            autoRepeatInterval: 200
        }

        // Value display
        Label {
            id: valueLabel
            width: 70
            horizontalAlignment: Text.AlignHCenter
            text: (root.value * 100).toFixed(0) + "%"
            color: "#ECEFF1"
            font.pixelSize: 20
            font.bold: true
        }

        // Minus button
        Button {
            id: minusBtn
            width: 70
            height: 50
            enabled: root.enabled && root.value > 0.0
            contentItem: Text {
                text: "−"
                font.pixelSize: 28
                font.bold: true
                color: "#FFFFFF"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            background: Rectangle {
                radius: 8
                color: minusBtn.pressed ? "#C62828" : minusBtn.enabled ? "#F44336" : "#EF9A9A"
                border.color: "#B71C1C"
                border.width: minusBtn.pressed ? 0 : 2
            }
            onClicked: {
                var newVal = Math.max(0.0, root.value - 0.1);
                root.value = newVal;
                root.valueChanged(newVal);
            }
            autoRepeat: true
            autoRepeatInterval: 200
        }

        // Stop button
        Button {
            id: stopBtn
            width: 100
            height: 50
            enabled: root.enabled && root.value > 0.0
            contentItem: Text {
                text: "STOP"
                font.pixelSize: 16
                font.bold: true
                color: "#FFFFFF"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            background: Rectangle {
                radius: 8
                color: stopBtn.pressed ? "#B71C1C" : "#D32F2F"
                border.color: "#B71C1C"
                border.width: 2
            }
            onClicked: {
                root.value = 0.0;
                root.valueChanged(0.0);
                root.stopRequested();
            }
        }
    }
}