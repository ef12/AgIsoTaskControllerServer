import QtQuick
import AgIsoTc 1.0
import "components"

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
            ctx.fillStyle = Theme.mapBg.toString();
            ctx.fillRect(0, 0, w, h);

            // Background grid: 10 m, with a stronger line every 50 m (50 m and 250 m when zoomed out)
            var step = 10 * root.scale;
            if (step < 8) step = 50 * root.scale;
            var major = step * 5;
            var ox = (w / 2 - root.viewX * root.scale) % step;
            var oy = (h / 2 - root.viewZ * root.scale) % step;
            ctx.strokeStyle = Theme.mapGrid.toString();
            ctx.lineWidth = 1;
            ctx.beginPath();
            for (var gx = ox; gx < w; gx += step) { ctx.moveTo(gx, 0); ctx.lineTo(gx, h); }
            for (var gy = oy; gy < h; gy += step) { ctx.moveTo(0, gy); ctx.lineTo(w, gy); }
            ctx.stroke();
            var mx = (w / 2 - root.viewX * root.scale) % major;
            var my = (h / 2 - root.viewZ * root.scale) % major;
            ctx.strokeStyle = Theme.mapGridMajor.toString();
            ctx.beginPath();
            for (var mgx = mx; mgx < w; mgx += major) { ctx.moveTo(mgx, 0); ctx.lineTo(mgx, h); }
            for (var mgy = my; mgy < h; mgy += major) { ctx.moveTo(0, mgy); ctx.lineTo(w, mgy); }
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
                var edge = bridge.boundaryRecording ? Theme.recording : Theme.accent;
                ctx.fillStyle = Theme.alpha(edge, 0.08).toString();
                ctx.fill();
                ctx.strokeStyle = edge.toString();
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
                ctx.fillStyle = Theme.alpha(Theme.info, 0.1).toString();
                ctx.strokeStyle = Theme.info.toString();
                ctx.lineWidth = 2;
                if (root.draftPoints.length >= 3) ctx.fill();
                ctx.stroke();
                for (var hp = 0; hp < root.draftPoints.length; ++hp) {
                    var handle = root.draftPoints[hp];
                    var hx = root.toScreenX(handle.x), hy = root.toScreenZ(handle.z);
                    ctx.fillStyle = hp === root.selectedDraftPoint ? Theme.warning.toString() : "#ffffff";
                    ctx.strokeStyle = Theme.info.toString();
                    ctx.lineWidth = 2;
                    ctx.beginPath();
                    ctx.arc(hx, hy, hp === root.selectedDraftPoint ? 7 : 5, 0, Math.PI * 2);
                    ctx.fill();
                    ctx.stroke();
                }
            }

            // TC coverage: the ground each section applied, one patch per straight stretch
            var patches = bridge.coveragePatches;
            if (patches && patches.length > 0) {
                ctx.fillStyle = Theme.alpha(Theme.coverage, 0.6).toString();
                for (var k = 0; k < patches.length; ++k) {
                    var cp = patches[k];
                    var px = root.toScreenX(cp.x), py = root.toScreenZ(cp.z);
                    var reach = (Math.max(cp.length, cp.width) / 2) * root.scale + 20;
                    if (px < -reach || py < -reach || px > w + reach || py > h + reach) continue;
                    ctx.save();
                    ctx.translate(px, py);
                    ctx.rotate((cp.course || 0) * Math.PI / 180);
                    var pw = Math.max(1, cp.width * root.scale);
                    var pl = Math.max(1, cp.length * root.scale);
                    ctx.fillRect(-pw / 2, -pl / 2, pw, pl);
                    ctx.restore();
                }
            }

            if (bridge.gpsValid) {
                // Trailed implement (rectangle, heading of implement), while one is connected
                if (bridge.implementReady) {
                    var ix = root.toScreenX(bridge.implementX), iy = root.toScreenZ(bridge.implementZ);
                    ctx.save();
                    ctx.translate(ix, iy);
                    ctx.rotate(bridge.implementCourse * Math.PI / 180);
                    var il = 6 * root.scale, iw = 3 * root.scale;
                    ctx.fillStyle = "#3a4149";
                    ctx.strokeStyle = "#9aa3ad";
                    ctx.lineWidth = 1.5;
                    ctx.beginPath();
                    ctx.rect(-iw / 2, -il / 2, iw, il);
                    ctx.fill();
                    ctx.stroke();
                    // Hitch bar toward tractor
                    ctx.strokeStyle = "#9aa3ad";
                    ctx.beginPath();
                    ctx.moveTo(0, -il / 2);
                    ctx.lineTo(0, -il / 2 - 2.8 * root.scale);
                    ctx.stroke();
                    ctx.restore();
                }

                // Tractor top view: front = driving direction = up at course 0.
                var px = root.toScreenX(bridge.tractorX), py = root.toScreenZ(bridge.tractorZ);
                ctx.save();
                ctx.translate(px, py);
                ctx.rotate(bridge.gpsCourse * Math.PI / 180);
                var tractorLength = 4.8 * root.scale;
                var tractorWidth = 2.6 * root.scale;
                var rearWheel = 0.9 * root.scale;
                var frontWheel = 0.62 * root.scale;

                ctx.fillStyle = "#c9631a";
                ctx.strokeStyle = "#1a1d21";
                ctx.lineWidth = 1.2;
                ctx.fillRect(-tractorWidth * 0.38, -tractorLength * 0.42, tractorWidth * 0.76, tractorLength * 0.82);
                ctx.strokeRect(-tractorWidth * 0.38, -tractorLength * 0.42, tractorWidth * 0.76, tractorLength * 0.82);
                ctx.fillStyle = "#ec7a1e";
                ctx.fillRect(-tractorWidth * 0.27, -tractorLength * 0.58, tractorWidth * 0.54, tractorLength * 0.34);
                ctx.fillStyle = "#a9d4ee";
                ctx.globalAlpha = 0.82;
                ctx.fillRect(-tractorWidth * 0.31, tractorLength * 0.05, tractorWidth * 0.62, tractorLength * 0.27);
                ctx.globalAlpha = 1.0;
                ctx.fillStyle = "#f4f6f8";
                ctx.fillRect(-tractorWidth * 0.2, -tractorLength * 0.67, tractorWidth * 0.4, tractorLength * 0.08);

                function wheel(x, y, w, h, steer) {
                    ctx.save();
                    ctx.translate(x, y);
                    ctx.rotate(steer * Math.PI / 180);
                    ctx.fillStyle = "#111417";
                    ctx.fillRect(-w / 2, -h / 2, w, h);
                    ctx.fillStyle = "#9aa3ad";
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
                ctx.strokeStyle = Theme.textMuted.toString();
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

    // Zoom and follow
    GlassPanel {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 12
        width: 44
        height: zoomTools.implicitHeight + 8

        Column {
            id: zoomTools
            anchors.centerIn: parent
            spacing: 2
            AppButton {
                variant: "ghost"
                iconName: "plus"
                focusPolicy: Qt.NoFocus
                tip: "Zoom in"
                onClicked: { root.scale = Math.min(30, root.scale * 1.25); root.repaint() }
            }
            AppButton {
                variant: "ghost"
                iconName: "minus"
                focusPolicy: Qt.NoFocus
                tip: "Zoom out"
                onClicked: { root.scale = Math.max(0.5, root.scale / 1.25); root.repaint() }
            }
            AppButton {
                variant: "ghost"
                iconName: "navigation"
                focusPolicy: Qt.NoFocus
                active: root.follow
                tip: root.follow ? "Following the tractor — click to pan freely" : "Follow the tractor"
                onClicked: root.follow = !root.follow
            }
        }
    }
}
