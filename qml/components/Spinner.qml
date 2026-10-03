import QtQuick
import QtQuick.Shapes
import AgIsoTc 1.0

// A turning arc that says "busy".
Item {
    id: root

    property real size: 20
    property color color: Theme.accent
    property bool running: true

    implicitWidth: size
    implicitHeight: size

    Rectangle {
        anchors.fill: parent
        radius: width / 2
        color: "transparent"
        border.width: 2
        border.color: Theme.alpha(root.color, 0.2)
    }

    Item {
        id: arc
        anchors.fill: parent
        layer.enabled: true
        layer.samples: 4

        Shape {
            anchors.fill: parent
            ShapePath {
                strokeColor: root.color
                strokeWidth: 2
                fillColor: "transparent"
                capStyle: ShapePath.RoundCap
                startX: root.size / 2
                startY: 1
                PathArc {
                    x: root.size - 1
                    y: root.size / 2
                    radiusX: root.size / 2 - 1
                    radiusY: root.size / 2 - 1
                }
            }
        }

        RotationAnimator on rotation {
            running: root.running && root.visible
            from: 0
            to: 360
            duration: 900
            loops: Animation.Infinite
        }
    }
}
