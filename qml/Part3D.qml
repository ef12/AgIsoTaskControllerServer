import QtQuick
import QtQuick3D

// A box part of the 3D vehicles: size in metres (sx across, sy up, sz along), a colour, and
// optionally a glow (lamps) or transparency (glass).
Model {
    id: part

    property real sx: 1
    property real sy: 1
    property real sz: 1
    property color color: "#5d646c"
    property vector3d glow: Qt.vector3d(0, 0, 0)
    property real alpha: 1
    property real metal: 0.2
    property real rough: 0.5
    property string shape: "#Cube"

    source: shape
    scale: Qt.vector3d(sx / 100, sy / 100, sz / 100)
    materials: PrincipledMaterial {
        baseColor: part.color
        emissiveFactor: part.glow
        opacity: part.alpha
        alphaMode: part.alpha < 1 ? PrincipledMaterial.Blend : PrincipledMaterial.Opaque
        metalness: part.metal
        roughness: part.rough
    }
}
