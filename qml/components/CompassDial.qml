import QtQuick
import QtQuick.Shapes
import AgIsoTc 1.0

// A compass rose whose needle points along the course (degrees clockwise from north).
Item {
    id: root

    property real course: 0
    property real size: 56
    property bool active: true

    implicitWidth: size
    implicitHeight: size

    Rectangle {
        anchors.fill: parent
        radius: width / 2
        color: Theme.inputBg
        border.color: Theme.border
    }

    Repeater {
        model: 12
        delegate: Item {
            anchors.fill: parent
            rotation: index * 30
            Rectangle {
                x: (parent.width - width) / 2
                y: 3
                width: index % 3 === 0 ? 2 : 1
                height: index % 3 === 0 ? 5 : 3
                radius: 1
                color: index === 0 ? Theme.danger : Theme.textMuted
                opacity: index === 0 ? 1 : 0.7
            }
        }
    }

    Item {
        anchors.fill: parent
        rotation: root.course
        opacity: root.active ? 1 : 0.35
        layer.enabled: true
        layer.samples: 4
        Behavior on rotation { RotationAnimation { duration: 240; direction: RotationAnimation.Shortest } }

        Shape {
            anchors.fill: parent
            ShapePath {
                strokeWidth: -1
                fillColor: Theme.accent
                startX: root.size / 2
                startY: root.size * 0.16
                PathLine { x: root.size * 0.62; y: root.size * 0.58 }
                PathLine { x: root.size / 2; y: root.size * 0.5 }
                PathLine { x: root.size * 0.38; y: root.size * 0.58 }
                PathLine { x: root.size / 2; y: root.size * 0.16 }
            }
        }
    }

    Rectangle {
        anchors.centerIn: parent
        width: 5
        height: 5
        radius: 2.5
        color: Theme.text
    }
}
