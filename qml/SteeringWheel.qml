import QtQuick
import QtQuick.Controls

Item {
    id: root
    property double value: 0.0 // -1.0 to 1.0
    property double diameter: 200
    property bool enabled: true
    property color wheelColor: "#37474F"
    property color handleColor: "#FF9800"

    signal valueChanged(double value)

    width: diameter
    height: diameter

    // Wheel background
    Canvas {
        anchors.fill: parent
        onPaint: {
            var ctx = getContext("2d");
            var centerX = width / 2;
            var centerY = height / 2;
            var radius = width / 2 - 10;

            // Outer ring
            ctx.beginPath();
            ctx.arc(centerX, centerY, radius, 0, 2 * Math.PI);
            ctx.fillStyle = wheelColor;
            ctx.fill();

            // Inner ring
            ctx.beginPath();
            ctx.arc(centerX, centerY, radius * 0.6, 0, 2 * Math.PI);
            ctx.fillStyle = "#263238";
            ctx.fill();

            // Center hub
            ctx.beginPath();
            ctx.arc(centerX, centerY, 20, 0, 2 * Math.PI);
            ctx.fillStyle = "#455A64";
            ctx.fill();

            // Tick marks (every 15 degrees)
            ctx.strokeStyle = "#546E7A";
            ctx.lineWidth = 2;
            for (var i = 0; i < 24; i++) {
                var angle = (i * 15 - 90) * Math.PI / 180;
                var r1 = radius - 5;
                var r2 = radius - 20;
                var x1 = width/2 + r1 * Math.cos(angle);
                var y1 = height/2 + r1 * Math.sin(angle);
                var x2 = width/2 + r2 * Math.cos(angle);
                var y2 = height/2 + r2 * Math.sin(angle);
                ctx.beginPath();
                ctx.moveTo(x1, y1);
                ctx.lineTo(x2, y2);
                ctx.stroke();
            }

            // Center mark (0 position)
            ctx.fillStyle = "#FF9800";
            ctx.beginPath();
            ctx.moveTo(centerX, centerY - radius + 5);
            ctx.lineTo(centerX - 5, centerY - radius + 20);
            ctx.lineTo(centerX + 5, centerY - radius + 20);
            ctx.closePath();
            ctx.fill();
        }
    }

    // Handle/knob
    Item {
        id: handle
        width: 60
        height: 60
        anchors.centerIn: parent

        // Calculate rotation angle based on value (-1 to 1 = -90 to +90 degrees from top)
        property double rotationAngle: value * 90

        transform: Rotation {
            origin.x: width/2
            origin.y: height/2
            angle: handle.rotationAngle
        }

        Canvas {
            anchors.fill: parent
            onPaint: {
                var ctx = getContext("2d");
                var centerX = width / 2;
                var centerY = height / 2;

                // Handle grip
                ctx.beginPath();
                ctx.arc(centerX, centerY, 25, 0, 2 * Math.PI);
                ctx.fillStyle = handleColor;
                ctx.fill();

                // Inner
                ctx.beginPath();
                ctx.arc(centerX, centerY, 15, 0, 2 * Math.PI);
                ctx.fillStyle = "#F57C00";
                ctx.fill();

                // Grip texture lines
                ctx.strokeStyle = "#E65100";
                ctx.lineWidth = 1;
                for (var i = 0; i < 8; i++) {
                    var angle = (i * 45) * Math.PI / 180;
                    var r1 = 16;
                    var r2 = 23;
                    var x1 = width/2 + r1 * Math.cos(angle);
                    var y1 = height/2 + r1 * Math.sin(angle);
                    var x2 = width/2 + r2 * Math.cos(angle);
                    var y2 = height/2 + r2 * Math.sin(angle);
                    ctx.beginPath();
                    ctx.moveTo(x1, y1);
                    ctx.lineTo(x2, y2);
                    ctx.stroke();
                }
            }
        }

        // Touch/mouse handling
        MouseArea {
            anchors.fill: parent
            enabled: root.enabled
            drag.target: handle
            drag.axis: Drag.XandYAxis
            drag.minimumX: -parent.width/2 + width/2
            drag.maximumX: parent.width/2 - width/2
            drag.minimumY: -parent.height/2 + height/2
            drag.maximumY: parent.height/2 - height/2

            onPositionChanged: {
                // Convert drag position to angle
                var dx = handle.x + handle.width/2 - parent.width/2;
                var dy = handle.y + handle.height/2 - parent.height/2;
                var angle = Math.atan2(dy, dx) * 180 / Math.PI;

                // Clamp to -90 to +90 (with 0 at top = -90 degrees in math coords)
                var relativeAngle = angle + 90; // 0 at top
                if (relativeAngle > 180) relativeAngle -= 360;

                // Clamp to ±90 degrees
                relativeAngle = Math.max(-90, Math.min(90, relativeAngle));

                // Constrain handle to arc
                var clampedRad = relativeAngle * Math.PI / 180;
                var r = parent.width/2 - 35;
                handle.x = parent.width/2 + r * Math.cos(clampedRad - Math.PI/2) - handle.width/2;
                handle.y = parent.height/2 + r * Math.sin(clampedRad - Math.PI/2) - handle.height/2;

                // Emit normalized value (-1 to 1)
                var newValue = relativeAngle / 90.0;
                if (Math.abs(newValue - root.value) > 0.01) {
                    root.value = newValue;
                    root.valueChanged(newValue);
                }
            }

            onReleased: {
                // Snap back to center if near center, otherwise keep position
                if (Math.abs(root.value) < 0.1) {
                    root.value = 0;
                    root.valueChanged(0);
                }
            }
        }
    }

    // Value display
    Label {
        anchors.top: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.topMargin: 8
        text: "Steering: " + (value * 100).toFixed(0) + "%"
        color: "#ECEFF1"
        font.pixelSize: 14
        font.bold: true
    }
}