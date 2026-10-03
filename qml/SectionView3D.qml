import QtQuick
import QtQuick.Layouts
import QtQuick3D
import QtQuick3D.Helpers
import AgIsoTc 1.0
import "components"

// The field in 3D: the tractor and the implement with its booms as LED bars, the field, the
// coverage and the boundary, under a sky. Heads-up displays float over it: the implement, the
// camera tools, the telemetry and, for the simulated GPS, the drive pad.
// Mouse: drag to orbit, right/Shift-drag to pan, wheel to zoom, double-click to follow.
// Keyboard (click the view first): W/S throttle, A/D steer, Space stop, C centre, F follow.
FocusScope {
    id: root

    signal openFieldMapRequested()
    signal showPageRequested(string key)

    property real cameraYaw: 35
    property real cameraPitch: -48
    property real cameraDistance: 180
    property real targetX: 0
    property real targetZ: 0
    property bool followTractor: true
    property real lastMouseX: 0
    property real lastMouseY: 0

    readonly property bool simulatedDriving: bridge.gpsRunning && bridge.gpsSourceText === "Simulated"

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

    function follow() {
        followTractor = true
        focusTractor()
    }

    // Glides the camera to a new view.
    function flyTo(yaw, pitch, distance, x, z) {
        cameraAnimation.stop()
        yawAnimation.to = yaw
        pitchAnimation.to = pitch
        distanceAnimation.to = distance
        targetXAnimation.to = x
        targetZAnimation.to = z
        cameraAnimation.start()
    }

    function fitField() {
        followTractor = false
        flyTo(35, -48, Math.max(80, Math.max(bridge.fieldWidthM, bridge.fieldLengthM) * 1.15), 0, 0)
    }

    function resetView() {
        followTractor = true
        flyTo(35, -48, 180, bridge.tractorX, bridge.tractorZ)
    }

    function zoomBy(factor) {
        zoomAnimation.to = Math.max(12, Math.min(5000, (zoomAnimation.running ? zoomAnimation.to : cameraDistance) * factor))
        zoomAnimation.restart()
    }

    ParallelAnimation {
        id: cameraAnimation
        NumberAnimation { id: yawAnimation; target: root; property: "cameraYaw"; duration: 520; easing.type: Easing.InOutCubic }
        NumberAnimation { id: pitchAnimation; target: root; property: "cameraPitch"; duration: 520; easing.type: Easing.InOutCubic }
        NumberAnimation { id: distanceAnimation; target: root; property: "cameraDistance"; duration: 520; easing.type: Easing.InOutCubic }
        NumberAnimation { id: targetXAnimation; target: root; property: "targetX"; duration: 520; easing.type: Easing.InOutCubic }
        NumberAnimation { id: targetZAnimation; target: root; property: "targetZ"; duration: 520; easing.type: Easing.InOutCubic }
    }
    NumberAnimation {
        id: zoomAnimation
        target: root
        property: "cameraDistance"
        duration: 200
        easing.type: Easing.OutCubic
    }

    Connections {
        target: bridge
        function onGpsChanged() {
            root.addTravel()
            if (root.followTractor && !cameraAnimation.running)
                root.focusTractor()
        }
    }

    Keys.onPressed: function(event) {
        if (event.key === Qt.Key_F) {
            root.follow()
            event.accepted = true
            return
        }
        if (!root.simulatedDriving)
            return
        switch (event.key) {
        case Qt.Key_W:
        case Qt.Key_Up:
            bridge.adjustThrottle(1)
            break
        case Qt.Key_S:
        case Qt.Key_Down:
            bridge.adjustThrottle(-1)
            break
        case Qt.Key_A:
        case Qt.Key_Left:
            bridge.setSteeringAngle(Math.max(-40, bridge.steeringAngle - 4))
            break
        case Qt.Key_D:
        case Qt.Key_Right:
            bridge.setSteeringAngle(Math.min(40, bridge.steeringAngle + 4))
            break
        case Qt.Key_Space:
            bridge.stopTractor()
            break
        case Qt.Key_C:
            bridge.setSteeringAngle(0)
            break
        default:
            return
        }
        event.accepted = true
    }

    Rectangle {
        anchors.fill: parent
        radius: Theme.radiusLg
        color: Theme.skyHorizon
    }

    View3D {
        id: view
        anchors.fill: parent

        environment: SceneEnvironment {
            backgroundMode: SceneEnvironment.SkyBox
            lightProbe: Texture {
                textureData: ProceduralSkyTextureData {
                    skyTopColor: Theme.skyTop
                    skyHorizonColor: Theme.skyHorizon
                    groundBottomColor: Theme.groundFar
                    groundHorizonColor: Theme.skyHorizon
                    sunEnergy: Theme.dark ? 0 : 1
                    textureQuality: ProceduralSkyTextureData.SkyTextureQualityMedium
                }
            }
            probeExposure: Theme.dark ? 0.45 : 0.6
            antialiasingMode: SceneEnvironment.MSAA
            antialiasingQuality: SceneEnvironment.High
            fog: Fog {
                enabled: true
                color: Theme.skyHorizon
                depthEnabled: true
                depthNear: root.cameraDistance * 2.2
                depthFar: root.cameraDistance * 9
                depthCurve: 1.2
            }
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
            brightness: Theme.dark ? 1.25 : 1.35
            castsShadow: true
            shadowFactor: 45
            shadowMapQuality: Light.ShadowMapQualityHigh
        }
        DirectionalLight {
            eulerRotation: Qt.vector3d(-25, 150, 0)
            brightness: 0.35
        }

        // the ground, and the selected field on it
        Model {
            source: "#Rectangle"
            y: -0.15
            eulerRotation.x: -90
            scale: Qt.vector3d(40, 40, 1)
            materials: PrincipledMaterial { baseColor: Theme.ground; roughness: 1.0 }
        }
        Model {
            source: "#Rectangle"
            y: -0.05
            eulerRotation.x: -90
            scale: Qt.vector3d(Math.max(0.1, bridge.fieldWidthM / 100),
                               Math.max(0.1, bridge.fieldLengthM / 100), 1)
            materials: PrincipledMaterial {
                baseColor: bridge.selectedFieldIndex >= 0 ? Theme.fieldActive : Theme.fieldIdle
                roughness: 0.96
            }
        }

        // Ground grid over the ground and the field: a fine line every 10 m and a
        // stronger one every 50 m, on the world origin, so the field's edges lie on it.
        Repeater3D {
            model: [{ spacing: 10, major: false, lift: -0.03 },
                    { spacing: 50, major: true, lift: -0.025 }]
            delegate: Model {
                required property var modelData
                y: modelData.lift
                geometry: GridGeometry { spacing: modelData.spacing; extent: 1000 }
                materials: PrincipledMaterial {
                    lighting: PrincipledMaterial.NoLighting
                    baseColor: modelData.major ? Theme.grid50 : Theme.grid10
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
                    materials: PrincipledMaterial {
                        baseColor: bridge.boundaryRecording ? Theme.recording : Theme.boundaryMarker
                        emissiveFactor: bridge.boundaryRecording ? Qt.vector3d(0.6, 0.3, 0.0) : Qt.vector3d(0, 0, 0)
                    }
                }
            }
        }

        Node {
            // TC coverage: the ground each section applied, one patch per straight stretch.
            // A list model updated in place: new patches add models, the rest are kept.
            Repeater3D {
                model: bridge.coveragePatchModel
                delegate: Model {
                    source: "#Cube"
                    position: Qt.vector3d(model.x, 0.04, model.z)
                    eulerRotation.y: -model.course
                    scale: Qt.vector3d(Math.max(0.002, model.width / 100), 0.0008,
                                       Math.max(0.002, model.length / 100))
                    materials: PrincipledMaterial { baseColor: Theme.coverage; opacity: 0.75; roughness: 1.0 }
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
                        emissiveFactor: (!rail && model.on) ? Qt.vector3d(0.12, 0.62, 0.24)
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
        cursorShape: pressed ? Qt.ClosedHandCursor : Qt.ArrowCursor
        onPressed: function(mouse) {
            root.forceActiveFocus()
            cameraAnimation.stop()
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
            root.zoomBy(wheel.angleDelta.y > 0 ? 0.86 : 1.16)
            wheel.accepted = true
        }
        onDoubleClicked: root.follow()
    }

    // rounded frame
    CornerMask {
        radius: Theme.radiusLg
        color: Theme.bg
    }
    Rectangle {
        anchors.fill: parent
        radius: Theme.radiusLg
        color: "transparent"
        border.color: root.activeFocus && root.simulatedDriving ? Theme.alpha(Theme.accent, 0.55) : Theme.border
        Behavior on border.color { ColorAnimation { duration: Theme.normal } }
    }

    // --- implement ---------------------------------------------------------------------------
    GlassPanel {
        id: implementChip
        x: 12
        y: 12
        width: Math.min(360, chipRow.implicitWidth + 24)
        height: 52

        RowLayout {
            id: chipRow
            anchors.fill: parent
            anchors.leftMargin: 10
            anchors.rightMargin: 12
            spacing: 10

            Rectangle {
                implicitWidth: 32
                implicitHeight: 32
                radius: 8
                color: bridge.booms.length > 0 ? Theme.alpha(Theme.accent, 0.16) : Theme.surfacePressed
                Icon {
                    anchors.centerIn: parent
                    name: "tractor"
                    size: 18
                    color: bridge.booms.length > 0 ? Theme.accentText : Theme.textMuted
                }
            }
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 0
                Text {
                    Layout.fillWidth: true
                    text: bridge.implementName
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSmall
                    font.weight: Font.DemiBold
                    color: Theme.text
                    elide: Text.ElideRight
                }
                Text {
                    Layout.fillWidth: true
                    text: bridge.implementGeometryStatus
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontCaption
                    color: Theme.textMuted
                    elide: Text.ElideRight
                }
            }
        }
    }

    // --- camera tools ------------------------------------------------------------------------
    GlassPanel {
        id: toolbar
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 12
        width: 44
        height: tools.implicitHeight + 8

        Column {
            id: tools
            anchors.centerIn: parent
            spacing: 2

            AppButton {

                focusPolicy: Qt.NoFocus
                variant: "ghost"
                iconName: "navigation"
                active: root.followTractor
                tip: root.followTractor ? "Following the tractor" : "Follow the tractor (F, or double-click)"
                onClicked: root.follow()
            }
            AppButton {
                focusPolicy: Qt.NoFocus
                variant: "ghost"
                iconName: "scan"
                tip: "Fit the field in view"
                onClicked: root.fitField()
            }
            AppButton {
                focusPolicy: Qt.NoFocus
                variant: "ghost"
                iconName: "plus"
                tip: "Zoom in"
                onClicked: root.zoomBy(0.7)
            }
            AppButton {
                focusPolicy: Qt.NoFocus
                variant: "ghost"
                iconName: "minus"
                tip: "Zoom out"
                onClicked: root.zoomBy(1.4)
            }
            AppButton {
                focusPolicy: Qt.NoFocus
                variant: "ghost"
                iconName: "rotateCcw"
                tip: "Reset the view"
                onClicked: root.resetView()
            }
            Rectangle {
                anchors.horizontalCenter: parent.horizontalCenter
                width: 20
                height: 1
                color: Theme.hudBorder
            }
            AppButton {
                focusPolicy: Qt.NoFocus
                variant: "ghost"
                iconName: "map"
                tip: "Open the field map"
                onClicked: root.openFieldMapRequested()
            }
            AppButton {
                focusPolicy: Qt.NoFocus
                variant: "ghost"
                iconName: "info"
                tip: "Drag to orbit · right-drag or Shift-drag to pan · scroll to zoom · double-click to follow the tractor"
            }
        }
    }

    // --- telemetry ---------------------------------------------------------------------------
    GlassPanel {
        id: telemetry
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.margins: 12
        width: telemetryRow.implicitWidth + 28
        height: 64

        RowLayout {
            id: telemetryRow
            anchors.centerIn: parent
            spacing: 16

            // without a position: say so, and offer the way to one
            RowLayout {
                visible: !bridge.gpsValid
                spacing: 12
                StatusBadge {
                    text: bridge.gpsRunning ? "Searching for GPS" : "GPS off"
                    tone: bridge.gpsRunning ? "warning" : "neutral"
                    pulse: bridge.gpsRunning
                }
                AppButton {
                    focusPolicy: Qt.NoFocus
                    visible: !bridge.gpsRunning
                    size: "sm"
                    text: "Set up GPS"
                    iconName: "locate"
                    onClicked: root.showPageRequested("gps")
                }
            }

            CompassDial {
                visible: bridge.gpsValid
                size: 40
                course: bridge.gpsCourse
            }
            Metric {
                visible: bridge.gpsValid
                label: "Speed"
                value: bridge.gpsSpeedKph.toFixed(1)
                unit: "km/h"
            }
            Rectangle {
                visible: bridge.gpsValid && root.width > 860
                Layout.preferredWidth: 1
                Layout.preferredHeight: 30
                color: Theme.hudBorder
            }
            Metric {
                visible: bridge.gpsValid && root.width > 860
                label: "Heading"
                value: bridge.gpsCourse.toFixed(0) + "°"
            }
            Rectangle {
                visible: bridge.gpsValid
                Layout.preferredWidth: 1
                Layout.preferredHeight: 30
                color: Theme.hudBorder
            }
            Metric {
                visible: bridge.gpsValid
                label: "Worked"
                value: bridge.workedAreaHa.toFixed(2)
                unit: "ha"
                valueColor: bridge.workedAreaHa > 0 ? Theme.accentText : Theme.text
            }
            Rectangle {
                visible: bridge.gpsValid && root.width > 760
                Layout.preferredWidth: 1
                Layout.preferredHeight: 30
                color: Theme.hudBorder
            }
            Metric {
                visible: bridge.gpsValid && root.width > 760
                label: "Sections"
                value: bridge.activeSectionCount + "/" + bridge.sectionCount
                valueColor: bridge.activeSectionCount > 0 ? Theme.accentText : Theme.text
            }
        }
    }

    // --- driving -----------------------------------------------------------------------------
    DrivePad {
        visible: root.simulatedDriving
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 12
        width: implicitWidth
        height: implicitHeight
        keyboardActive: root.activeFocus
    }
}
