import QtQuick
import QtQuick.Shapes
import AgIsoTc 1.0
import "Icons.js" as Icons

// A line icon from Icons.js in any size and colour. The Shape is drawn multisampled into a
// layer, so its curves stay smooth.
Item {
    id: root

    property string name
    property color color: Theme.textSecondary
    property real size: 18
    property real stroke: 2

    readonly property var glyph: Icons.glyph(name)

    implicitWidth: size
    implicitHeight: size
    layer.enabled: glyph.d !== ""
    layer.samples: 4
    layer.smooth: true

    Shape {
        width: 24
        height: 24
        scale: root.size / 24
        transformOrigin: Item.TopLeft
        visible: root.glyph.d !== ""

        ShapePath {
            strokeColor: root.color
            strokeWidth: root.stroke
            fillColor: root.glyph.fill ? root.color : "transparent"
            capStyle: ShapePath.RoundCap
            joinStyle: ShapePath.RoundJoin
            PathSvg { path: root.glyph.d }
        }
    }
}
