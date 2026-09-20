import QtQuick
import QtQuick.Controls

// Top-down 2D field map. All coordinates are bridge local meters:
// x = east, z = -north (north is up on screen). Heading/course is
// degrees clockwise from north; Canvas rotation matches it directly
// for shapes drawn pointing up (-y).
Item {
    id: root

    property real scale: 3.0 // pixels per meter
    property bool follow: true

    property real viewX: bridge.tractorX
    property real viewZ: bridge.tractorZ

    function toScreenX(x) { return width / 2 + (x - viewX) * scale; }
    function toScreenZ(z) { return height / 2 + (z - viewZ) * scale; }

    Timer {
        interval: 200
        running: true
        repeat: true
        onTriggered: {
            if (root.follow && bridge.gpsValid) {
                viewX = bridge.tractorX;
                viewZ = bridge.tractorZ;
            }
            mapCanvas.requestPaint();
        }
    }

    Canvas {
        id: mapCanvas
        anchors.fill: parent

        onPaint: {
            var ctx = getContext("2d");
            var w = width, h = height;
            ctx.clearRect(0, 0, w, h);

            // Background grid (10 m spacing)
            ctx.strokeStyle = "#242b34";
            ctx.lineWidth = 1;
            var step = 10 * root.scale;
            if (step < 8) step = 50 * root.scale;
            var ox = (w / 2 - root.viewX * root.scale) % step;
            var oy = (h / 2 - root.viewZ * root.scale) % step;
            ctx.beginPath();
            for (var gx = ox; gx < w; gx += step) { ctx.moveTo(gx, 0); ctx.lineTo(gx, h); }
            for (var gy = oy; gy < h; gy += step) { ctx.moveTo(0, gy); ctx.lineTo(w, gy); }
            ctx.stroke();

            // Field boundary polygon
            var boundary = bridge.fieldBoundaryPoints;
            if (boundary && boundary.length >= 3) {
                ctx.beginPath();
                var first = true;
                for (var i = 0; i < boundary.length; i++) {
                    var bp = boundary[i];
                    var sx = root.toScreenX(bp.x), sy = root.toScreenZ(bp.z);
                    if (first) { ctx.moveTo(sx, sy); first = false; }
                    else { ctx.lineTo(sx, sy); }
                }
                ctx.closePath();
                ctx.fillStyle = bridge.boundaryRecording ? "rgba(242, 211, 60, 0.08)" : "rgba(53, 199, 89, 0.08)";
                ctx.fill();
                ctx.strokeStyle = bridge.boundaryRecording ? "#f2d33c" : "#35c759";
                ctx.lineWidth = 2;
                ctx.stroke();
            }

            // Worked swaths (decimated for speed)
            var worked = bridge.workedPoints;
            if (worked && worked.length > 0) {
                var stride = Math.max(1, Math.ceil(worked.length / 900));
                ctx.fillStyle = "rgba(53, 199, 89, 0.55)";
                for (var k = 0; k < worked.length; k += stride) {
                    var wp = worked[k];
                    var wx = root.toScreenX(wp.x), wy = root.toScreenZ(wp.z);
                    if (wx < -20 || wy < -20 || wx > w + 20 || wy > h + 20) continue;
                    ctx.save();
                    ctx.translate(wx, wy);
                    ctx.rotate((wp.course || 0) * Math.PI / 180);
                    var sw = Math.max(1.5, (wp.width || 3) * root.scale);
                    ctx.fillRect(-sw / 2, -0.4 * root.scale, sw, 0.8 * root.scale);
                    ctx.restore();
                }
            }

            // Track history polyline (decimated)
            var track = bridge.trackPoints;
            if (track && track.length > 1) {
                var tstride = Math.max(1, Math.ceil(track.length / 700));
                ctx.beginPath();
                var started = false;
                for (var t = 0; t < track.length; t += tstride) {
                    var tp = track[t];
                    var tx = root.toScreenX(tp.x), ty = root.toScreenZ(tp.z);
                    if (!started) { ctx.moveTo(tx, ty); started = true; }
                    else { ctx.lineTo(tx, ty); }
                }
                ctx.strokeStyle = "rgba(115, 199, 255, 0.8)";
                ctx.lineWidth = 1.5;
                ctx.stroke();
            }

            // Trailed implement (rectangle, heading of implement)
            if (bridge.gpsValid) {
                var ix = root.toScreenX(bridge.implementX), iy = root.toScreenZ(bridge.implementZ);
                ctx.save();
                ctx.translate(ix, iy);
                ctx.rotate(bridge.implementCourse * Math.PI / 180);
                var il = 6 * root.scale, iw = 3 * root.scale;
                ctx.fillStyle = "#8a5a1e";
                ctx.strokeStyle = "#f0a832";
                ctx.lineWidth = 1.5;
                ctx.beginPath();
                ctx.rect(-iw / 2, -il / 2, iw, il);
                ctx.fill();
                ctx.stroke();
                // Hitch bar toward tractor
                ctx.strokeStyle = "#c7d0dc";
                ctx.beginPath();
                ctx.moveTo(0, -il / 2);
                ctx.lineTo(0, -il / 2 - 2.8 * root.scale);
                ctx.stroke();
                ctx.restore();

                // Tractor top view: front = driving direction = up at course 0.
                var px = root.toScreenX(bridge.tractorX), py = root.toScreenZ(bridge.tractorZ);
                ctx.save();
                ctx.translate(px, py);
                ctx.rotate(bridge.gpsCourse * Math.PI / 180);
                var tractorLength = 4.8 * root.scale;
                var tractorWidth = 2.6 * root.scale;
                var rearWheel = 0.9 * root.scale;
                var frontWheel = 0.62 * root.scale;

                ctx.fillStyle = "#237a34";
                ctx.strokeStyle = "#dce8f5";
                ctx.lineWidth = 1.2;
                ctx.fillRect(-tractorWidth * 0.38, -tractorLength * 0.42, tractorWidth * 0.76, tractorLength * 0.82);
                ctx.strokeRect(-tractorWidth * 0.38, -tractorLength * 0.42, tractorWidth * 0.76, tractorLength * 0.82);
                ctx.fillStyle = "#2f9d45";
                ctx.fillRect(-tractorWidth * 0.27, -tractorLength * 0.58, tractorWidth * 0.54, tractorLength * 0.34);
                ctx.fillStyle = "#8dd9ff";
                ctx.globalAlpha = 0.82;
                ctx.fillRect(-tractorWidth * 0.31, tractorLength * 0.05, tractorWidth * 0.62, tractorLength * 0.27);
                ctx.globalAlpha = 1.0;
                ctx.fillStyle = "#e6d74a";
                ctx.fillRect(-tractorWidth * 0.2, -tractorLength * 0.67, tractorWidth * 0.4, tractorLength * 0.08);

                function wheel(x, y, w, h, steer) {
                    ctx.save();
                    ctx.translate(x, y);
                    ctx.rotate(steer * Math.PI / 180);
                    ctx.fillStyle = "#111417";
                    ctx.fillRect(-w / 2, -h / 2, w, h);
                    ctx.fillStyle = "#f0c33c";
                    ctx.fillRect(-w * 0.22, -h * 0.22, w * 0.44, h * 0.44);
                    ctx.restore();
                }
                wheel(-tractorWidth * 0.62, tractorLength * 0.22, rearWheel, rearWheel * 0.42, 0);
                wheel( tractorWidth * 0.62, tractorLength * 0.22, rearWheel, rearWheel * 0.42, 0);
                wheel(-tractorWidth * 0.52, -tractorLength * 0.40, frontWheel, frontWheel * 0.38, bridge.steeringAngle);
                wheel( tractorWidth * 0.52, -tractorLength * 0.40, frontWheel, frontWheel * 0.38, bridge.steeringAngle);
                ctx.restore();
            }

            // Center crosshair when not following
            if (!root.follow) {
                ctx.strokeStyle = "#59636f";
                ctx.lineWidth = 1;
                ctx.beginPath();
                ctx.moveTo(w / 2 - 8, h / 2); ctx.lineTo(w / 2 + 8, h / 2);
                ctx.moveTo(w / 2, h / 2 - 8); ctx.lineTo(w / 2, h / 2 + 8);
                ctx.stroke();
            }
        }
    }

    // Pan by dragging when not following
    MouseArea {
        anchors.fill: parent
        enabled: !root.follow
        property real lastX: 0
        property real lastY: 0
        onPressed: function (mouse) { lastX = mouse.x; lastY = mouse.y; }
        onPositionChanged: function (mouse) {
            root.viewX -= (mouse.x - lastX) / root.scale;
            root.viewZ -= (mouse.y - lastY) / root.scale;
            lastX = mouse.x; lastY = mouse.y;
        }
        onWheel: function (wheel) {
            if (wheel.angleDelta.y > 0) root.scale = Math.min(30, root.scale * 1.15);
            else root.scale = Math.max(0.5, root.scale / 1.15);
        }
    }

    // Zoom controls
    Column {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 8
        spacing: 6
        Button {
            text: "+"; width: 36; height: 36
            onClicked: root.scale = Math.min(30, root.scale * 1.25)
        }
        Button {
            text: "−"; width: 36; height: 36
            onClicked: root.scale = Math.max(0.5, root.scale / 1.25)
        }
        Button {
            text: "◎"; width: 36; height: 36
            checkable: true
            checked: root.follow
            onClicked: root.follow = !root.follow
        }
    }
}
