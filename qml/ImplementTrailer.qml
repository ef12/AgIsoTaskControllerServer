import QtQuick
import QtQuick3D

// The carrier the booms ride on: a tongue from the hitch, two frame beams, a cross beam under
// each boom's LED bar, a transport axle with two wheels under the middle, and a gauge wheel at
// each end of the rearmost boom. In metres, in the implement's frame: the hitch at the origin,
// z rearward, x to the right, y up from the ground. Shown once the client's booms are known.
//   booms:  bridge.booms (left, right, z of each)
//   travel: metres the implement drove, which turns the wheels
Node {
    id: trailer

    property var booms: []
    property real travel: 0

    readonly property color steel: "#3a4149"
    readonly property color dark: "#24292e"
    readonly property real hitchHeight: 0.55
    readonly property real frameHeight: 0.78        // the beams' centre; the LED bars rest on them
    readonly property real wheelRadius: 0.55
    readonly property real gaugeRadius: 0.34

    readonly property bool valid: booms.length > 0
    readonly property real left: valid ? Math.min.apply(null, booms.map(b => b.left)) : -1
    readonly property real right: valid ? Math.max.apply(null, booms.map(b => b.right)) : 1
    readonly property real frontZ: valid ? Math.min.apply(null, booms.map(b => b.z)) - 0.55 : 1
    readonly property real rearZ: valid ? Math.max.apply(null, booms.map(b => b.z)) + 0.55 : 2
    readonly property real centreX: (left + right) / 2
    readonly property real axleZ: (frontZ + rearZ) / 2

    visible: valid

    // tongue from the hitch pin up to the frame
    readonly property real tongueRise: frameHeight - hitchHeight
    readonly property real tongueLength: Math.hypot(frontZ, tongueRise)
    Part3D {
        sx: 0.2; sy: 0.16; sz: trailer.tongueLength
        x: trailer.centreX * 0.5
        z: trailer.frontZ / 2
        y: (trailer.hitchHeight + trailer.frameHeight) / 2
        eulerRotation.x: -Math.asin(trailer.tongueRise / Math.max(0.01, trailer.tongueLength)) * 180 / Math.PI
        eulerRotation.y: Math.atan2(trailer.centreX, trailer.frontZ) * 180 / Math.PI
        color: trailer.steel; metal: 0.5
    }
    Part3D { shape: "#Cylinder"; sx: 0.22; sy: 0.14; sz: 0.22; y: trailer.hitchHeight; color: trailer.dark; metal: 0.6 }

    // two frame beams along the trailer
    Repeater3D {
        model: [-0.65, 0.65]
        delegate: Part3D {
            required property var modelData
            sx: 0.16; sy: 0.16; sz: trailer.rearZ - trailer.frontZ
            x: trailer.centreX + modelData
            z: (trailer.frontZ + trailer.rearZ) / 2
            y: trailer.frameHeight
            color: trailer.steel; metal: 0.5
        }
    }
    // a cross beam under each boom
    Repeater3D {
        model: trailer.booms
        delegate: Part3D {
            required property var modelData
            sx: modelData.right - modelData.left; sy: 0.12; sz: 0.18
            x: (modelData.left + modelData.right) / 2
            z: modelData.z
            y: trailer.frameHeight
            color: trailer.steel; metal: 0.5
        }
    }

    // transport axle with two wheels under the middle of the frame
    Part3D { sx: 2.5; sy: 0.12; sz: 0.12; x: trailer.centreX; z: trailer.axleZ; y: trailer.wheelRadius; color: trailer.dark }
    Repeater3D {
        model: [-1.3, 1.3]
        delegate: Node {
            required property var modelData
            x: trailer.centreX + modelData; z: trailer.axleZ; y: trailer.wheelRadius
            // the leg from the frame down to the hub
            Part3D { sx: 0.12; sy: trailer.frameHeight - trailer.wheelRadius; sz: 0.14
                     x: modelData < 0 ? 0.3 : -0.3; y: (trailer.frameHeight - trailer.wheelRadius) / 2
                     color: trailer.steel }
            WheelModel {
                radius: trailer.wheelRadius; tyreWidth: 0.36; lugs: 14
                rimColor: "#c5cad0"; hubColor: "#8d949c"
                spin: trailer.travel / trailer.wheelRadius * 180 / Math.PI
            }
        }
    }

    // gauge wheels at both ends of the rearmost boom
    Repeater3D {
        model: [0.45, -0.45]
        delegate: Node {
            required property var modelData
            readonly property real side: modelData > 0 ? trailer.left + modelData : trailer.right + modelData
            x: side; z: trailer.rearZ; y: trailer.gaugeRadius
            Part3D { sx: 0.08; sy: trailer.frameHeight - trailer.gaugeRadius; sz: 0.08
                     z: -0.3; y: (trailer.frameHeight - trailer.gaugeRadius) / 2; color: trailer.steel }
            Part3D { sx: 0.08; sy: 0.08; sz: 0.34; z: -0.15; color: trailer.steel }
            WheelModel {
                radius: trailer.gaugeRadius; tyreWidth: 0.16; lugs: 10
                spin: trailer.travel / trailer.gaugeRadius * 180 / Math.PI
            }
        }
    }
}
