import QtQuick
import QtQuick.Templates as T
import AgIsoTc 1.0

// The simulated tractor's steering wheel: drag it round to steer, -40° (left) to 40° (right).
// It follows the steering angle however it is set (keyboard, centre button).
T.Dial {
    id: wheel

    from: -40
    to: 40
    stepSize: 1
    snapMode: T.Dial.SnapAlways
    wrap: false
    focusPolicy: Qt.NoFocus
    value: bridge.steeringAngle
    implicitWidth: 112
    implicitHeight: 112

    onMoved: bridge.setSteeringAngle(value)
    onValueChanged: canvas.requestPaint()
    onPressedChanged: canvas.requestPaint()

    Connections {
        target: bridge
        function onDrivingControlsChanged() {
            if (!wheel.pressed)
                wheel.value = bridge.steeringAngle
        }
    }
    Connections {
        target: Theme
        function onDarkChanged() { canvas.requestPaint() }
    }

    handle: null

    background: Canvas {
        id: canvas
        implicitWidth: 112
        implicitHeight: 112

        onPaint: {
            const ctx = getContext("2d")
            const w = width, h = height
            const radius = Math.min(w, h) / 2 - 7
            ctx.reset()
            ctx.translate(w / 2, h / 2)
            ctx.rotate(wheel.value * 2.4 * Math.PI / 180)

            // rim
            const rim = ctx.createRadialGradient(0, 0, radius * 0.7, 0, 0, radius + 6)
            rim.addColorStop(0, "#3a434d")
            rim.addColorStop(0.6, "#15191e")
            rim.addColorStop(1, wheel.pressed ? Theme.accent.toString() : "#07090b")
            ctx.lineWidth = 11
            ctx.strokeStyle = rim
            ctx.beginPath()
            ctx.arc(0, 0, radius, 0, Math.PI * 2)
            ctx.stroke()

            // grip highlight
            ctx.lineWidth = 1.5
            ctx.strokeStyle = "rgba(255, 255, 255, 0.10)"
            ctx.beginPath()
            ctx.arc(0, 0, radius - 4, Math.PI * 1.1, Math.PI * 1.9)
            ctx.stroke()

            // three spokes: left, right and down
            ctx.strokeStyle = "#5c6672"
            ctx.lineWidth = 7
            ctx.lineCap = "round"
            for (let i = 0; i < 3; ++i) {
                const angle = i * 90 * Math.PI / 180
                ctx.beginPath()
                ctx.moveTo(Math.cos(angle) * 14, Math.sin(angle) * 14)
                ctx.lineTo(Math.cos(angle) * (radius - 6), Math.sin(angle) * (radius - 6))
                ctx.stroke()
            }

            // hub
            ctx.fillStyle = "#1a1f25"
            ctx.strokeStyle = Theme.accent.toString()
            ctx.lineWidth = 2
            ctx.beginPath()
            ctx.arc(0, 0, 16, 0, Math.PI * 2)
            ctx.fill()
            ctx.stroke()

            // top-dead-centre marker, so the turn is visible
            ctx.fillStyle = Theme.accent.toString()
            ctx.beginPath()
            ctx.arc(0, -radius, 3.5, 0, Math.PI * 2)
            ctx.fill()
        }
    }
}
