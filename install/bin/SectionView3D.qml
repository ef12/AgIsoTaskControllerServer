import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick3D

GroupBox {
    title: "Section control 3D"
    ColumnLayout {
        anchors.fill: parent
        spacing: 6
        View3D {
            Layout.fillWidth: true
            Layout.fillHeight: true
            environment: SceneEnvironment {
                clearColor: "#14171c"
                backgroundMode: SceneEnvironment.Color
            }
            PerspectiveCamera {
                position: Qt.vector3d(0, 170, 260)
                eulerRotation: Qt.vector3d(-33, 0, 0)
            }
            DirectionalLight {
                eulerRotation: Qt.vector3d(-45, -30, 0)
                brightness: 1.2
            }
            // Ground plane.
            Model {
                source: "#Rectangle"
                eulerRotation: Qt.vector3d(-90, 0, 0)
                scale: Qt.vector3d(9, 9, 9)
                materials: PrincipledMaterial {
                    baseColor: "#20242b"
                    roughness: 0.9
                }
            }
            // One box per section; color follows the section-state DDI.
            Node {
                Repeater3D {
                    model: bridge.sectionStates
                    delegate: Model {
                        source: "#Cube"
                        position: Qt.vector3d((index - (bridge.sectionCount - 1) / 2) * 24, 6, 0)
                        scale: Qt.vector3d(0.2, 0.12, 0.7)
                        materials: PrincipledMaterial {
                            baseColor: modelData ? "#35c759" : "#3a4048"
                            roughness: 0.4
                        }
                    }
                }
            }
        }
        RowLayout {
            spacing: 6
            Label {
                text: "Section-state DDI (0 = off):"
                color: "#c7d0dc"
            }
            SpinBox {
                id: sectionDdiSpin
                from: 0
                to: 65535
                value: 0
                editable: true
                Layout.preferredWidth: 110
                onValueChanged: bridge.setSectionDdi(value)
            }
            Label {
                text: "Sections:"
                color: "#c7d0dc"
            }
            SpinBox {
                id: sectionCountSpin
                from: 1
                to: 64
                value: 16
                editable: true
                Layout.preferredWidth: 90
                onValueChanged: bridge.setSectionCount(value)
            }
            Item {
                Layout.fillWidth: true
            }
        }
    }
}
