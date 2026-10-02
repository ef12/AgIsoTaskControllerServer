import QtQuick
import QtQuick3D

// A wheel for the 3D view: tyre with tread lugs, rim, hub and spokes, its axle along X and its
// centre at the node's position. spin turns it about the axle, in degrees (positive rolls it
// toward -Z, the way the vehicles in the scene drive). All sizes are in metres.
Node {
    id: wheel

    property real radius: 0.5
    property real tyreWidth: 0.4
    property real spin: 0
    property color tyreColor: "#1b1e22"
    property color rimColor: "#c5cad0"
    property color hubColor: "#8d949c"
    property int lugs: 16

    readonly property real rimRadius: radius * 0.62

    Node {
        eulerRotation.x: -wheel.spin

        // the tyre, a little smaller than the radius so the lugs stand out
        Model {
            source: "#Cylinder"
            eulerRotation.z: 90
            scale: Qt.vector3d((wheel.radius - 0.035) / 50, wheel.tyreWidth / 100, (wheel.radius - 0.035) / 50)
            materials: PrincipledMaterial { baseColor: wheel.tyreColor; roughness: 0.95 }
        }
        // the rim, showing on both sides
        Model {
            source: "#Cylinder"
            eulerRotation.z: 90
            scale: Qt.vector3d(wheel.rimRadius / 50, wheel.tyreWidth * 1.04 / 100, wheel.rimRadius / 50)
            materials: PrincipledMaterial { baseColor: wheel.rimColor; metalness: 0.55; roughness: 0.35 }
        }
        // the hub
        Model {
            source: "#Cylinder"
            eulerRotation.z: 90
            scale: Qt.vector3d(wheel.radius * 0.2 / 50, wheel.tyreWidth * 1.12 / 100, wheel.radius * 0.2 / 50)
            materials: PrincipledMaterial { baseColor: wheel.hubColor; metalness: 0.6; roughness: 0.3 }
        }
        // spokes across the rim, so the turning shows
        Repeater3D {
            model: 3
            delegate: Model {
                required property int index
                source: "#Cube"
                eulerRotation.x: index * 60
                scale: Qt.vector3d(wheel.tyreWidth * 1.08 / 100, wheel.rimRadius * 1.9 / 100, wheel.radius * 0.07 / 100)
                materials: PrincipledMaterial { baseColor: wheel.hubColor; metalness: 0.5; roughness: 0.4 }
            }
        }
        // tread lugs around the tyre
        Repeater3D {
            model: wheel.lugs
            delegate: Model {
                required property int index
                readonly property real angle: index * 360 / wheel.lugs
                source: "#Cube"
                position: Qt.vector3d(0, (wheel.radius - 0.03) * Math.cos(angle * Math.PI / 180),
                                      (wheel.radius - 0.03) * Math.sin(angle * Math.PI / 180))
                eulerRotation.x: angle
                scale: Qt.vector3d(wheel.tyreWidth * 0.92 / 100, 0.07 / 100, wheel.radius * 0.24 / 100)
                materials: PrincipledMaterial { baseColor: wheel.tyreColor; roughness: 0.95 }
            }
        }
    }
}
