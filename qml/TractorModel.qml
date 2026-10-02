import QtQuick
import QtQuick3D

// A large row-crop tractor with a cab, in metres, front toward -Z, standing on the ground at
// y = 0 with the tractor's reference point at the origin; the hitch pin is 2.8 m behind it,
// where the implement pivots (TcBridge::updateTrailerPose). Orange bonnet and fenders, dark
// chassis and cab frame, glass all round, rear wheels of 2 m and steered front wheels.
//   travel:   metres driven, which turns the wheels
//   steering: front wheel angle in degrees, negative to the left
Node {
    id: tractor

    property real travel: 0
    property real steering: 0

    readonly property color paint: "#ec7a1e"
    readonly property color chassis: "#2c3136"
    readonly property color frame: "#17191c"
    readonly property real rearRadius: 1.0
    readonly property real frontRadius: 0.76
    readonly property real rearAxleZ: 1.25
    readonly property real frontAxleZ: -1.45

    // --- chassis, axles, front weight ----------------------------------------------------------
    Part3D { sx: 0.86; sy: 0.75; sz: 3.15; z: -0.675; y: 0.95; color: tractor.chassis }
    Part3D { sx: 1.62; sy: 0.2; sz: 0.26; z: tractor.frontAxleZ; y: tractor.frontRadius; color: tractor.chassis }
    Part3D { sx: 1.5; sy: 0.32; sz: 0.42; z: tractor.rearAxleZ; y: tractor.rearRadius; color: tractor.chassis }
    Part3D { sx: 1.02; sy: 0.46; sz: 0.32; z: -2.62; y: 0.95; color: tractor.chassis; metal: 0.5 }

    // --- bonnet with a rounded top, grille and headlights ---------------------------------------
    Part3D { sx: 1.05; sy: 0.72; sz: 2.25; z: -1.275; y: 1.62; color: tractor.paint; metal: 0.3; rough: 0.35 }
    Part3D {
        shape: "#Cylinder"
        eulerRotation.x: 90
        sx: 1.05; sy: 2.25; sz: 0.34
        z: -1.275; y: 1.98
        color: tractor.paint; metal: 0.3; rough: 0.35
    }
    Part3D { sx: 0.9; sy: 0.6; sz: 0.05; z: -2.41; y: 1.58; color: "#101215" }
    Part3D { sx: 0.24; sy: 0.1; sz: 0.04; x: -0.31; z: -2.44; y: 1.84; color: "#f4f6f8"; glow: Qt.vector3d(0.9, 0.9, 0.85) }
    Part3D { sx: 0.24; sy: 0.1; sz: 0.04; x: 0.31; z: -2.44; y: 1.84; color: "#f4f6f8"; glow: Qt.vector3d(0.9, 0.9, 0.85) }

    // --- exhaust ------------------------------------------------------------------------------
    Part3D { shape: "#Cylinder"; sx: 0.12; sy: 1.25; sz: 0.12; x: 0.42; z: -0.42; y: 2.55; color: tractor.frame; metal: 0.6 }

    // --- cab: lower body, glass, pillars, roof with lamps and a beacon --------------------------
    Part3D { sx: 1.76; sy: 0.36; sz: 1.72; z: 0.6; y: 1.86; color: tractor.chassis }
    Part3D { sx: 1.64; sy: 1.0; sz: 1.6; z: 0.6; y: 2.55; color: "#a9d4ee"; alpha: 0.32; rough: 0.05 }
    Repeater3D {
        model: [Qt.vector2d(-0.83, -0.2), Qt.vector2d(0.83, -0.2), Qt.vector2d(-0.83, 1.4), Qt.vector2d(0.83, 1.4)]
        delegate: Part3D {
            required property var modelData
            sx: 0.09; sy: 1.06; sz: 0.09
            x: modelData.x; z: modelData.y; y: 2.55
            color: tractor.frame
        }
    }
    Part3D { sx: 1.92; sy: 0.16; sz: 1.88; z: 0.6; y: 3.13; color: tractor.frame; rough: 0.6 }
    Part3D { sx: 1.92; sy: 0.08; sz: 0.14; z: -0.33; y: 3.07; color: tractor.paint }
    Part3D { sx: 0.16; sy: 0.08; sz: 0.06; x: -0.7; z: -0.38; y: 3.02; color: "#f4f6f8"; glow: Qt.vector3d(0.9, 0.9, 0.85) }
    Part3D { sx: 0.16; sy: 0.08; sz: 0.06; x: 0.7; z: -0.38; y: 3.02; color: "#f4f6f8"; glow: Qt.vector3d(0.9, 0.9, 0.85) }
    Part3D { shape: "#Cylinder"; sx: 0.16; sy: 0.14; sz: 0.16; z: 0.9; y: 3.28; color: "#ffb020"; glow: Qt.vector3d(0.8, 0.45, 0.0) }

    // --- rear fenders over the rear wheels ------------------------------------------------------
    Repeater3D {
        model: [-1, 1]
        delegate: Node {
            required property var modelData
            x: modelData * 0.98
            Part3D { sx: 0.72; sy: 0.07; sz: 1.62; z: tractor.rearAxleZ; y: 2.1; color: tractor.paint; metal: 0.3; rough: 0.35 }
            Part3D { sx: 0.72; sy: 0.42; sz: 0.07; z: tractor.rearAxleZ - 0.8; y: 1.9; color: tractor.paint; metal: 0.3; rough: 0.35 }
        }
    }

    // --- wheels: the rear ones turn with the distance, the front ones also steer -----------------
    WheelModel {
        x: -0.98; z: tractor.rearAxleZ; y: tractor.rearRadius
        radius: tractor.rearRadius; tyreWidth: 0.62; lugs: 22
        spin: tractor.travel / tractor.rearRadius * 180 / Math.PI
    }
    WheelModel {
        x: 0.98; z: tractor.rearAxleZ; y: tractor.rearRadius
        radius: tractor.rearRadius; tyreWidth: 0.62; lugs: 22
        spin: tractor.travel / tractor.rearRadius * 180 / Math.PI
    }
    Repeater3D {
        model: [-1, 1]
        delegate: Node {
            required property var modelData
            x: modelData * 0.92; z: tractor.frontAxleZ; y: tractor.frontRadius
            eulerRotation.y: -tractor.steering
            WheelModel {
                radius: tractor.frontRadius; tyreWidth: 0.5; lugs: 18
                spin: tractor.travel / tractor.frontRadius * 180 / Math.PI
            }
            // mudguard, steering with the wheel
            Part3D { sx: 0.56; sy: 0.05; sz: 1.0; y: tractor.frontRadius + 0.14; color: tractor.frame }
        }
    }

    // --- rear linkage and the drawbar to the hitch pin ------------------------------------------
    Part3D { sx: 0.1; sy: 0.1; sz: 0.75; x: -0.45; z: 1.75; y: 0.72; color: tractor.chassis }
    Part3D { sx: 0.1; sy: 0.1; sz: 0.75; x: 0.45; z: 1.75; y: 0.72; color: tractor.chassis }
    Part3D { sx: 0.2; sy: 0.12; sz: 1.5; z: 2.05; y: 0.55; color: tractor.chassis; metal: 0.5 }
    Part3D { shape: "#Cylinder"; sx: 0.12; sy: 0.24; sz: 0.12; z: 2.8; y: 0.55; color: "#9aa1a9"; metal: 0.7 }
}
