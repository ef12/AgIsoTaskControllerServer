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
    property bool drawMode: false
    property var draftPoints: []
    property int selectedDraftPoint: -1
    property real nudgeStepM: 0.1

    property real viewX: bridge.tractorX
    property real viewZ: bridge.tractorZ

    function toScreenX(x) { return width / 2 + (x - viewX) * scale; }
    function toScreenZ(z) { return height / 2 + (z - viewZ) * scale; }
    function fromScreenX(x) { return viewX + (x - width / 2) / scale; }
    function fromScreenZ(y) { return viewZ + (y - height / 2) / scale; }
    function repaint() { mapCanvas.requestPaint(); }
    function clearDraft() {
        draftPoints = [];
        selectedDraftPoint = -1;
        repaint();
    }
    function addDraftPoint(localX, localZ) {
        var points = draftPoints.slice();
        points.push({ "x": localX, "z": localZ });
        draftPoints = points;
        selectedDraftPoint = points.length - 1;
        repaint();
    }
    function moveSelectedDraftPoint(dx, dz) {
        if (selectedDraftPoint < 0 || selectedDraftPoint >= draftPoints.length)
            return;
        var points = draftPoints.slice();
        points[selectedDraftPoint] = {
            "x": points[selectedDraftPoint].x + dx,
            "z": points[selectedDraftPoint].z + dz
        };
        draftPoints = points;
        repaint();
    }
    function nearestDraftPoint(screenX, screenY) {
        var best = -1;
        var bestDistance = 12 * 12;
        for (var i = 0; i < draftPoints.length; ++i) {
            var dx = toScreenX(draftPoints[i].x) - screenX;
            var dy = toScreenZ(draftPoints[i].z) - screenY;
            var distance = dx * dx + dy * dy;
            if (distance < bestDistance) {
                bestDistance = distance;
                best = i;
            }
        }
        return best;
    }

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

            // Draft field polygon drawn by mouse.
            if (root.draftPoints.length > 0) {
                ctx.beginPath();
                for (var dp = 0; dp < root.draftPoints.length; ++dp) {
                    var draft = root.draftPoints[dp];
                    var dsx = root.toScreenX(draft.x), dsy = root.toScreenZ(draft.z);
                    if (dp === 0) ctx.moveTo(dsx, dsy);
                    else ctx.lineTo(dsx, dsy);
                }
                if (root.draftPoints.length >= 3)
                    ctx.closePath();
                ctx.fillStyle = "rgba(115, 199, 255, 0.08)";
                ctx.strokeStyle = "#73c7ff";
                ctx.lineWidth = 2;
                if (root.draftPoints.length >= 3) ctx.fill();
                ctx.stroke();
                for (var hp = 0; hp < root.draftPoints.length; ++hp) {
                    var handle = root.draftPoints[hp];
                    var hx = root.toScreenX(handle.x), hy = root.toScreenZ(handle.z);
                    ctx.fillStyle = hp === root.selectedDraftPoint ? "#f2d33c" : "#dce8f5";
                    ctx.strokeStyle = "#0d1117";
                    ctx.lineWidth = 2;
                    ctx.beginPath();
                    ctx.arc(hx, hy, hp === root.selectedDraftPoint ? 7 : 5, 0, Math.PI * 2);
                    ctx.fill();
                    ctx.stroke();
                }
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

    MouseArea {
        id: mapMouse
        anchors.fill: parent
        focus: true
        enabled: true
        property real lastX: 0
        property real lastY: 0
        property bool draggingDraftPoint: false
        onPressed: function (mouse) {
            forceActiveFocus();
            lastX = mouse.x;
            lastY = mouse.y;
            draggingDraftPoint = false;
            if (root.drawMode) {
                var nearest = root.nearestDraftPoint(mouse.x, mouse.y);
                if (nearest >= 0) {
                    root.selectedDraftPoint = nearest;
                    draggingDraftPoint = true;
                } else {
                    root.addDraftPoint(root.fromScreenX(mouse.x), root.fromScreenZ(mouse.y));
                }
                mouse.accepted = true;
            }
        }
        onPositionChanged: function (mouse) {
            if (root.drawMode && draggingDraftPoint && root.selectedDraftPoint >= 0) {
                var points = root.draftPoints.slice();
                points[root.selectedDraftPoint] = {
                    "x": root.fromScreenX(mouse.x),
                    "z": root.fromScreenZ(mouse.y)
                };
                root.draftPoints = points;
                root.repaint();
            } else if (!root.follow && (mouse.buttons & Qt.LeftButton)) {
                root.viewX -= (mouse.x - lastX) / root.scale;
                root.viewZ -= (mouse.y - lastY) / root.scale;
                root.repaint();
            }
            lastX = mouse.x; lastY = mouse.y;
        }
        onReleased: draggingDraftPoint = false
        onWheel: function (wheel) {
            if (wheel.angleDelta.y > 0) root.scale = Math.min(30, root.scale * 1.15);
            else root.scale = Math.max(0.5, root.scale / 1.15);
            root.repaint();
        }
        Keys.onPressed: function(event) {
            if (!root.drawMode)
                return;
            if (event.key === Qt.Key_Left) {
                root.moveSelectedDraftPoint(-root.nudgeStepM, 0);
                event.accepted = true;
            } else if (event.key === Qt.Key_Right) {
                root.moveSelectedDraftPoint(root.nudgeStepM, 0);
                event.accepted = true;
            } else if (event.key === Qt.Key_Up) {
                root.moveSelectedDraftPoint(0, -root.nudgeStepM);
                event.accepted = true;
            } else if (event.key === Qt.Key_Down) {
                root.moveSelectedDraftPoint(0, root.nudgeStepM);
                event.accepted = true;
            } else if ((event.key === Qt.Key_Delete || event.key === Qt.Key_Backspace) &&
                       root.selectedDraftPoint >= 0 && root.selectedDraftPoint < root.draftPoints.length) {
                var points = root.draftPoints.slice();
                points.splice(root.selectedDraftPoint, 1);
                root.draftPoints = points;
                root.selectedDraftPoint = Math.min(root.selectedDraftPoint, points.length - 1);
                root.repaint();
                event.accepted = true;
            }
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
