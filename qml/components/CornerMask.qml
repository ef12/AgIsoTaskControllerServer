import QtQuick
import QtQuick.Shapes

// Paints the four corners outside a rounded rectangle in `color`, so content that cannot be
// clipped round (a View3D, a Canvas) looks like it sits in a rounded frame. Fills its parent.
Item {
    id: root

    property real radius: 12
    property color color: "black"

    anchors.fill: parent

    Repeater {
        model: 4
        delegate: Item {
            width: root.radius
            height: root.radius
            x: (index % 2) ? root.width - width : 0
            y: (index >= 2) ? root.height - height : 0
            rotation: [0, 90, 270, 180][index]
            layer.enabled: true
            layer.samples: 4

            Shape {
                anchors.fill: parent
                ShapePath {
                    strokeWidth: -1
                    fillColor: root.color
                    startX: 0
                    startY: 0
                    PathLine { x: root.radius; y: 0 }
                    PathArc {
                        x: 0
                        y: root.radius
                        radiusX: root.radius
                        radiusY: root.radius
                        direction: PathArc.Counterclockwise
                    }
                    PathLine { x: 0; y: 0 }
                }
            }
        }
    }
}
