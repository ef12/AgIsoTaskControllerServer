import QtQuick
import QtQuick.Controls
import QtQuick.Shapes

Item {
    id: root
    property double latitude: 0.0
    property double longitude: 0.0
    property double heading: 0.0
    property double speed: 0.0
    property var trackHistory: []
    property var fieldBoundaries: []
    property var currentBoundary: null
    property double centerLat: 52.0
    property double centerLon: 5.0
    property double zoomLevel: 18.0
    property bool showTrackHistory: true
    property bool showFieldBoundaries: true

    // Map projection helpers
    function latLonToXY(lat, lon, centerLat, centerLon, zoom) {
        // Simple Web Mercator-like projection for local display
        // 1 degree lat ≈ 111km, 1 degree lon ≈ 111km * cos(lat)
        var latRad = centerLat * Math.PI / 180.0;
        var metersPerDegLat = 111319.0;
        var metersPerDegLon = 111319.0 * Math.cos(latRad);
        var scale = Math.pow(2, zoom) / 256.0; // pixels per meter at zoom
        var dx = (lon - centerLon) * metersPerDegLon * scale;
        var dy = -(lat - centerLat) * metersPerDegLat * scale; // Y inverted for screen coords
        return { x: dx + width/2, y: dy + height/2 };
    }

    // Tractor icon
    Canvas {
        id: tractorCanvas
        width: 40
        height: 40
        visible: latitude !== 0 || longitude !== 0

        property double lat: latitude
        property double lon: longitude
        property double hdg: heading

        onLatChanged: requestPaint()
        onLonChanged: requestPaint()
        onHdgChanged: requestPaint()

        onPaint: {
            var ctx = getContext("2d");
            ctx.clearRect(0, 0, width, height);
            ctx.save();
            ctx.translate(width/2, height/2);
            ctx.rotate(hdg * Math.PI / 180.0);

            // Tractor body
            ctx.fillStyle = "#2196F3";
            ctx.beginPath();
            ctx.moveTo(0, -15); // Front
            ctx.lineTo(-10, 10); // Rear left
            ctx.lineTo(10, 10);  // Rear right
            ctx.closePath();
            ctx.fill();

            // Cab
            ctx.fillStyle = "#1976D2";
            ctx.beginPath();
            ctx.moveTo(-5, -5);
            ctx.lineTo(5, -5);
            ctx.lineTo(3, 5);
            ctx.lineTo(-3, 5);
            ctx.closePath();
            ctx.fill();

            // Front indicator
            ctx.fillStyle = "#FFEB3B";
            ctx.beginPath();
            ctx.moveTo(-3, -10);
            ctx.lineTo(3, -10);
            ctx.lineTo(0, -15);
            ctx.closePath();
            ctx.fill();

            ctx.restore();
        }

        // Position update
        Component.onCompleted: {
            var pos = root.latLonToXY(latitude, longitude, centerLat, centerLon, zoomLevel);
            x = pos.x - width/2;
            y = pos.y - height/2;
        }

        Binding {
            target: tractorCanvas
            property: "x"
            value: {
                var pos = root.latLonToXY(latitude, longitude, centerLat, centerLon, zoomLevel);
                return pos.x - width/2;
            }
            when: latitude !== 0 || longitude !== 0
        }

        Binding {
            target: tractorCanvas
            property: "y"
            value: {
                var pos = root.latLonToXY(latitude, longitude, centerLat, centerLon, zoomLevel);
                return pos.y - height/2;
            }
            when: latitude !== 0 || longitude !== 0
        }
    }

    // Implement icon
    Canvas {
        id: implementCanvas
        width: 50
        height: 30
        visible: latitude !== 0 || longitude !== 0

        property double lat: implementLat
        property double lon: implementLon
        property double hdg: implementHeading

        property double implementLat: 0
        property double implementLon: 0
        property double implementHeading: 0

        onPaint: {
            var ctx = getContext("2d");
            ctx.clearRect(0, 0, width, height);
            ctx.save();
            ctx.translate(width/2, height/2);
            ctx.rotate(hdg * Math.PI / 180.0);

            // Implement body (wagon-like)
            ctx.fillStyle = "#FF9800";
            ctx.beginPath();
            ctx.rect(-20, -10, 40, 20);
            ctx.fill();

            // Axles
            ctx.fillStyle = "#E65100";
            ctx.fillRect(-18, -12, 4, 24);
            ctx.fillRect(14, -12, 4, 24);

            // Hitch
            ctx.fillStyle = "#795548";
            ctx.fillRect(-20, -5, 8, 10);

            ctx.restore();
        }

        Binding {
            target: implementCanvas
            property: "implementLat"
            value: implementLat
        }
        Binding {
            target: implementCanvas
            property: "implementLon"
            value: implementLon
        }
        Binding {
            target: implementCanvas
            property: "implementHeading"
            value: implementHeading
        }

        Binding {
            target: implementCanvas
            property: "x"
            value: {
                var pos = root.latLonToXY(implementLat, implementLon, centerLat, centerLon, zoomLevel);
                return pos.x - width/2;
            }
            when: implementLat !== 0 || implementLon !== 0
        }
        Binding {
            target: implementCanvas
            property: "y"
            value: {
                var pos = root.latLonToXY(implementLat, implementLon, centerLat, centerLon, zoomLevel);
                return pos.y - height/2;
            }
            when: implementLat !== 0 || implementLon !== 0
        }
    }

    // Track history
    Repeater {
        model: showTrackHistory ? trackHistory : []
        delegate: Canvas {
            width: 6
            height: 6
            x: {
                var pos = root.latLonToXY(modelData.latitudeDeg, modelData.longitudeDeg, centerLat, centerLon, zoomLevel);
                return pos.x - width/2;
            }
            y: {
                var pos = root.latLonToXY(modelData.latitudeDeg, modelData.longitudeDeg, centerLat, centerLon, zoomLevel);
                return pos.y - height/2;
            }
            visible: latitudeDeg !== 0 && longitudeDeg !== 0
            onPaint: {
                var ctx = getContext("2d");
                ctx.fillStyle = "#4CAF50";
                ctx.globalAlpha = 0.6;
                ctx.beginPath();
                ctx.arc(width/2, height/2, 2, 0, 2*Math.PI);
                ctx.fill();
            }
        }
    }

    // Field boundaries
    Repeater {
        model: showFieldBoundaries ? fieldBoundaries : []
        delegate: Shape {
            ShapePath {
                strokeWidth: 2
                strokeColor: "#4CAF50"
                fillColor: "#4CAF50"
                fillOpacity: 0.1
                fillRule: ShapePath.OddEvenFill

                Path {
                    startX: 0
                    startY: 0
                    PathSvg { path: modelData.path }
                }
            }
        }
    }

    // Current boundary being recorded
    Shape {
        visible: currentBoundary && currentBoundary.vertices && currentBoundary.vertices.length > 1
        ShapePath {
            strokeWidth: 3
            strokeColor: "#FFC107"
            strokeStyle: ShapePath.DashLine
            dashPattern: [10, 5]

            Path {
                startX: {
                    var pos = root.latLonToXY(currentBoundary.vertices[0].first, currentBoundary.vertices[0].second, centerLat, centerLon, zoomLevel);
                    return pos.x - root.width/2;
                }
                startY: {
                    var pos = root.latLonToXY(currentBoundary.vertices[0].first, currentBoundary.vertices[0].second, centerLat, centerLon, zoomLevel);
                    return pos.y - root.height/2;
                }
                PathSvg {
                    path: {
                        var path = "";
                        for (var i = 1; i < currentBoundary.vertices.length; i++) {
                            var pos = root.latLonToXY(currentBoundary.vertices[i].first, currentBoundary.vertices[i].second, centerLat, centerLon, zoomLevel);
                            path += "L " + (pos.x - root.width/2) + " " + (pos.y - root.height/2) + " ";
                        }
                        return path;
                    }
                }
            }
        }
    }

    // Center crosshair
    Canvas {
        anchors.centerIn: parent
        width: 30
        height: 30
        visible: true
        onPaint: {
            var ctx = getContext("2d");
            ctx.strokeStyle = "#FFFFFF";
            ctx.lineWidth = 1;
            ctx.beginPath();
            ctx.moveTo(width/2 - 10, height/2);
            ctx.lineTo(width/2 + 10, height/2);
            ctx.moveTo(width/2, height/2 - 10);
            ctx.lineTo(width/2, height/2 + 10);
            ctx.stroke();
        }
    }
}