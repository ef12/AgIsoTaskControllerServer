import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick3D

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

                Node {
                    Repeater3D {
                        model: bridge.trackPoints
                        delegate: Model {
                            source: "#Sphere"
                            position: Qt.vector3d(modelData.x, 0.18, modelData.z)
                            scale: Qt.vector3d(0.007, 0.003, 0.007)
                            materials: PrincipledMaterial {
                                baseColor: "#34b7ff"
                                emissiveFactor: Qt.vector3d(0.08, 0.25, 0.4)
                            }
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
                    Repeater3D {
                        model: bridge.workedPoints
                        delegate: Model {
                            source: "#Cube"
                            position: Qt.vector3d(modelData.x, 0.04, modelData.z)
                            eulerRotation.y: modelData.course
                            scale: Qt.vector3d(Math.max(0.006, modelData.width / 100), 0.0008, 0.012)
                            materials: PrincipledMaterial { baseColor: "#7cb342"; opacity: 0.72; roughness: 1.0 }
                        }
                    }
                }

                Node {
                    position: Qt.vector3d(bridge.tractorX, 1.2, bridge.tractorZ)
                    eulerRotation.y: bridge.gpsCourse

                    // Tractor dimensions are represented in metres. Its rear hitch is at +Z.
                    Model {
                        source: "#Cube"
                        position: Qt.vector3d(0, 0.7, 0)
                        scale: Qt.vector3d(0.025, 0.018, 0.052)
                        materials: PrincipledMaterial { baseColor: "#35c759"; metalness: 0.1; roughness: 0.5 }
                    }
                    Model {
                        source: "#Cube"
                        position: Qt.vector3d(0, 2.15, 0.65)
                        scale: Qt.vector3d(0.026, 0.022, 0.027)
                        materials: PrincipledMaterial { baseColor: "#8dd9ff"; metalness: 0.25; roughness: 0.25 }
                    }
                    Model {
                        source: "#Cube"
                        position: Qt.vector3d(0, 0.4, -3.7)
                        scale: Qt.vector3d(0.022, 0.012, 0.022)
                        materials: PrincipledMaterial { baseColor: "#2fa84d"; roughness: 0.55 }
                    }
                    Model { source: "#Cylinder"; position: Qt.vector3d(-1.55, 0, -2.5); eulerRotation.z: 90; scale: Qt.vector3d(0.014, 0.007, 0.014); materials: PrincipledMaterial { baseColor: "#171a1d"; roughness: 0.9 } }
                    Model { source: "#Cylinder"; position: Qt.vector3d( 1.55, 0, -2.5); eulerRotation.z: 90; scale: Qt.vector3d(0.014, 0.007, 0.014); materials: PrincipledMaterial { baseColor: "#171a1d"; roughness: 0.9 } }
                    Model { source: "#Cylinder"; position: Qt.vector3d(-1.65, 0,  2.5); eulerRotation.z: 90; scale: Qt.vector3d(0.019, 0.008, 0.019); materials: PrincipledMaterial { baseColor: "#171a1d"; roughness: 0.9 } }
                    Model { source: "#Cylinder"; position: Qt.vector3d( 1.65, 0,  2.5); eulerRotation.z: 90; scale: Qt.vector3d(0.019, 0.008, 0.019); materials: PrincipledMaterial { baseColor: "#171a1d"; roughness: 0.9 } }
                }

                // The implement pivots around the tractor hitch and follows with its own heading.
                Node {
                        position: Qt.vector3d(bridge.implementX, 0.8, bridge.implementZ)
                        eulerRotation.y: bridge.implementCourse
                        Repeater3D {
                            model: bridge.implementElements
                            delegate: Model {
                                source: modelData.type === 6 ? "#Cylinder" : "#Cube"
                                position: Qt.vector3d(modelData.x,
                                                      Math.max(0.08, modelData.y + modelData.height / 2),
                                                      modelData.z)
                                eulerRotation.x: modelData.type === 6 ? 90 : 0
                                scale: Qt.vector3d(Math.max(0.002, modelData.width / 100),
                                                   Math.max(0.002, modelData.height / 100),
                                                   Math.max(0.002, modelData.length / 100))
                                materials: PrincipledMaterial {
                                    baseColor: modelData.active ? "#f2d33c"
                                                               : (modelData.type === 4 ? "#4ea8de"
                                                                                       : (modelData.type === 3 ? "#d58c3b" : "#69747f"))
                                    emissiveFactor: modelData.active ? Qt.vector3d(0.25, 0.20, 0.02) : Qt.vector3d(0, 0, 0)
                                    metalness: 0.25
                                    roughness: 0.55
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
                to: 64
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
