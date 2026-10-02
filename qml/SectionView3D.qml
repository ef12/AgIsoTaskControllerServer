import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick3D
import AgIsoTc 1.0

GroupBox {
    id: root
    title: "Field and section control 3D"

    property real cameraYaw: 35
    property real cameraPitch: -48
    property real cameraDistance: 360
    property real targetX: 0
    property real targetZ: 0
    property bool followTractor: true
    property real lastMouseX: 0
    property real lastMouseY: 0

    // Metres the tractor and the implement drove, which turn their wheels. A step longer than
    // a few metres between two updates is a jump of the position (a new field, the GPS
    // restarted), not driving, and is left out.
    property real tractorTravel: 0
    property real implementTravel: 0
    property real lastTractorX: NaN
    property real lastTractorZ: NaN
    property real lastImplementX: NaN
    property real lastImplementZ: NaN
    readonly property real maxTravelStepM: 5

    function addTravel() {
        const tractorStep = Math.hypot(bridge.tractorX - lastTractorX, bridge.tractorZ - lastTractorZ)
        if (isFinite(tractorStep) && tractorStep < maxTravelStepM)
            tractorTravel += tractorStep
        const implementStep = Math.hypot(bridge.implementX - lastImplementX, bridge.implementZ - lastImplementZ)
        if (isFinite(implementStep) && implementStep < maxTravelStepM)
            implementTravel += implementStep
        lastTractorX = bridge.tractorX
        lastTractorZ = bridge.tractorZ
        lastImplementX = bridge.implementX
        lastImplementZ = bridge.implementZ
    }

    function focusTractor() {
        targetX = bridge.tractorX
        targetZ = bridge.tractorZ
    }

    function fitField() {
        targetX = 0
        targetZ = 0
        cameraYaw = 35
        cameraPitch = -48
        cameraDistance = Math.max(80, Math.max(bridge.fieldWidthM, bridge.fieldLengthM) * 1.15)
    }

    Connections {
        target: bridge
        function onGpsChanged() {
            root.addTravel()
            if (root.followTractor)
                root.focusTractor()
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 6

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            View3D {
                anchors.fill: parent
                environment: SceneEnvironment {
                    clearColor: "#10161c"
                    backgroundMode: SceneEnvironment.Color
                    antialiasingMode: SceneEnvironment.MSAA
                    antialiasingQuality: SceneEnvironment.High
                }

                Node {
                    position: Qt.vector3d(root.targetX, 0, root.targetZ)
                    eulerRotation.y: root.cameraYaw
                    Node {
                        eulerRotation.x: root.cameraPitch
                        PerspectiveCamera {
                            z: root.cameraDistance
                            clipNear: 0.5
                            clipFar: 20000
                        }
                    }
                }

                DirectionalLight {
                    eulerRotation: Qt.vector3d(-55, -30, 0)
                    brightness: 1.35
                    castsShadow: true
                    shadowFactor: 35
                }
                DirectionalLight {
                    eulerRotation: Qt.vector3d(-25, 150, 0)
                    brightness: 0.45
                }

                Model {
                    source: "#Rectangle"
                    y: -0.15
                    eulerRotation.x: -90
                    scale: Qt.vector3d(20, 20, 1)
                    materials: PrincipledMaterial { baseColor: "#182127"; roughness: 1.0 }
                }
                Model {
                    source: "#Rectangle"
                    y: -0.05
                    eulerRotation.x: -90
                    scale: Qt.vector3d(Math.max(0.1, bridge.fieldWidthM / 100),
                                       Math.max(0.1, bridge.fieldLengthM / 100), 1)
                    materials: PrincipledMaterial {
                        baseColor: bridge.selectedFieldIndex >= 0 ? "#294d32" : "#202a31"
                        roughness: 0.96
                    }
                }

                // Ground grid over the ground and the field: a fine line every 10 m and a
                // stronger one every 50 m, on the world origin, so the field's edges lie on it.
                Repeater3D {
                    model: [{ spacing: 10, colour: "#34424f", lift: -0.03 },
                            { spacing: 50, colour: "#5b6f84", lift: -0.025 }]
                    delegate: Model {
                        required property var modelData
                        y: modelData.lift
                        geometry: GridGeometry { spacing: modelData.spacing; extent: 1000 }
                        materials: PrincipledMaterial {
                            lighting: PrincipledMaterial.NoLighting
                            baseColor: modelData.colour
                        }
                    }
                }


                Node {
                    Repeater3D {
                        model: bridge.fieldBoundaryPoints
                        delegate: Model {
                            source: "#Sphere"
                            position: Qt.vector3d(modelData.x, 0.3, modelData.z)
                            scale: Qt.vector3d(0.018, 0.018, 0.018)
                            materials: PrincipledMaterial { baseColor: bridge.boundaryRecording ? "#ff9f0a" : "#dce8f5" }
                        }
                    }
                }

                Node {
                    // TC coverage: the ground each section applied, one patch per straight stretch
                    // A list model updated in place: new patches add models, the rest are kept.
                    Repeater3D {
                        model: bridge.coveragePatchModel
                        delegate: Model {
                            source: "#Cube"
                            position: Qt.vector3d(model.x, 0.04, model.z)
                            eulerRotation.y: -model.course
                            scale: Qt.vector3d(Math.max(0.002, model.width / 100), 0.0008,
                                               Math.max(0.002, model.length / 100))
                            materials: PrincipledMaterial { baseColor: "#7cb342"; opacity: 0.72; roughness: 1.0 }
                        }
                    }
                }

                // The tractor at its reference point, heading the GPS course; its wheels turn with
                // the distance it drives, the front ones also with the steering.
                Node {
                    position: Qt.vector3d(bridge.tractorX, 0, bridge.tractorZ)
                    eulerRotation.y: -bridge.gpsCourse
                    TractorModel {
                        travel: root.tractorTravel
                        steering: bridge.steeringAngle
                    }
                }

                // The implement pivots around the tractor hitch and follows with its own heading.
                Node {
                        position: Qt.vector3d(bridge.implementX, 0.8, bridge.implementZ)
                        eulerRotation.y: -bridge.implementCourse
                        // The connector, and the booms that carry sections with their sections:
                        // drawn only until the booms are known. Then the trailer below carries the
                        // booms, drawn as LED bars.
                        Repeater3D {
                            model: bridge.implementElementModel
                            delegate: Model {
                                visible: bridge.boomLedModel.count === 0
                                source: model.type === 6 ? "#Cylinder" : "#Cube"
                                position: Qt.vector3d(model.x,
                                                      Math.max(0.08, model.y + model.height / 2),
                                                      model.z)
                                eulerRotation.x: model.type === 6 ? 90 : 0
                                scale: Qt.vector3d(Math.max(0.002, model.width / 100),
                                                   Math.max(0.002, model.height / 100),
                                                   Math.max(0.002, model.length / 100))
                                materials: PrincipledMaterial {
                                    baseColor: model.active ? "#f2d33c"
                                                           : (model.type === 4 ? "#4ea8de"
                                                                               : (model.type === 3 ? "#d58c3b" : "#69747f"))
                                    emissiveFactor: model.active ? Qt.vector3d(0.25, 0.20, 0.02) : Qt.vector3d(0, 0, 0)
                                    metalness: 0.25
                                    roughness: 0.55
                                }
                            }
                        }

                        // The trailer the booms ride on, from the hitch, with wheels on the ground
                        // (the implement's frame is 0.8 m up).
                        ImplementTrailer {
                            y: -0.8
                            booms: bridge.booms
                            travel: root.implementTravel
                        }

                        // Each boom as an LED bar trailing the tractor: a dark rail as wide as
                        // the boom, and on it one LED per section, as wide as the section, lit
                        // green while the section is on. Booms that would lie on top of each
                        // other are drawn one behind the other.
                        Repeater3D {
                            model: bridge.boomLedModel
                            delegate: Model {
                                readonly property bool rail: model.kind === "rail"
                                source: "#Cube"
                                position: Qt.vector3d(model.x, rail ? 0.16 : 0.40, model.z)
                                scale: Qt.vector3d(Math.max(0.002, model.width / 100),
                                                   rail ? 0.0024 : 0.0022,
                                                   rail ? 0.0085 : 0.0060)
                                materials: PrincipledMaterial {
                                    baseColor: rail ? "#2c333b" : (model.on ? "#3ddc6e" : "#1b2128")
                                    emissiveFactor: (!rail && model.on) ? Qt.vector3d(0.18, 0.85, 0.36)
                                                                         : Qt.vector3d(0, 0, 0)
                                    metalness: rail ? 0.4 : 0.05
                                    roughness: rail ? 0.6 : 0.3
                                }
                            }
                        }
                }
            }

            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                hoverEnabled: true
                onPressed: function(mouse) {
                    root.lastMouseX = mouse.x
                    root.lastMouseY = mouse.y
                }
                onPositionChanged: function(mouse) {
                    if (mouse.buttons === Qt.NoButton)
                        return
                    const dx = mouse.x - root.lastMouseX
                    const dy = mouse.y - root.lastMouseY
                    root.lastMouseX = mouse.x
                    root.lastMouseY = mouse.y
                    if ((mouse.buttons & Qt.RightButton) || (mouse.modifiers & Qt.ShiftModifier)) {
                        const scale = root.cameraDistance / 450
                        const radians = root.cameraYaw * Math.PI / 180
                        root.targetX -= dx * scale * Math.cos(radians) + dy * scale * Math.sin(radians)
                        root.targetZ += dx * scale * Math.sin(radians) - dy * scale * Math.cos(radians)
                        root.followTractor = false
                    } else {
                        root.cameraYaw = (root.cameraYaw - dx * 0.35) % 360
                        root.cameraPitch = Math.max(-88, Math.min(-8, root.cameraPitch - dy * 0.3))
                    }
                }
                onWheel: function(wheel) {
                    root.cameraDistance = Math.max(12, Math.min(5000,
                                               root.cameraDistance * (wheel.angleDelta.y > 0 ? 0.86 : 1.16)))
                    wheel.accepted = true
                }
                onDoubleClicked: {
                    root.followTractor = true
                    root.focusTractor()
                }
            }

            Rectangle {
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.margins: 8
                width: helpText.implicitWidth + 16
                height: helpText.implicitHeight + 10
                radius: 5
                color: "#b010161c"
                Label {
                    id: helpText
                    anchors.centerIn: parent
                    text: "Drag: orbit  •  Right/Shift-drag: pan  •  Wheel: zoom  •  Double-click: follow"
                    color: "#c7d0dc"
                    font.pixelSize: 11
                }
            }

            Rectangle {
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 8
                width: Math.min(parent.width * 0.55, implementText.implicitWidth + 20)
                height: implementText.implicitHeight + 12
                radius: 5
                color: "#b010161c"
                Label {
                    id: implementText
                    anchors.centerIn: parent
                    width: parent.width - 16
                    text: bridge.implementName + "\n" + bridge.implementGeometryStatus
                    color: "#dce8f5"
                    font.pixelSize: 11
                    elide: Text.ElideRight
                }
            }
        }

        RowLayout {
            spacing: 6
            Button {
                text: root.followTractor ? "Following tractor" : "Follow tractor"
                highlighted: root.followTractor
                onClicked: {
                    root.followTractor = true
                    root.focusTractor()
                }
            }
            Button {
                text: "Fit field"
                onClicked: {
                    root.followTractor = false
                    root.fitField()
                }
            }
            CheckBox {
                text: "Sync DDIs"
                checked: bridge.autoDdiSync
                onToggled: bridge.setAutoDdiSync(checked)
            }
            Button {
                text: "Request now"
                enabled: bridge.selectedClient >= 0
                onClicked: bridge.requestImplementDdis()
            }
            Label { text: "DDI:"; color: "#c7d0dc" }
            SpinBox {
                from: 0
                to: 65535
                value: 0
                editable: true
                Layout.preferredWidth: 100
                onValueChanged: bridge.setSectionDdi(value)
            }
            Label { text: "Sections:"; color: "#c7d0dc" }
            SpinBox {
                from: 1
                to: 96
                value: 16
                editable: true
                Layout.preferredWidth: 80
                onValueChanged: bridge.setSectionCount(value)
            }
            Item { Layout.fillWidth: true }
            Label {
                text: bridge.gpsValid ? bridge.gpsSpeedKph.toFixed(1) + " km/h  " + bridge.gpsCourse.toFixed(0) + "°" : "GPS offline"
                color: bridge.gpsValid ? "#35c759" : "#8995a3"
            }
        }
    }
}
