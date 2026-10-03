.pragma library

// Line icons on a 24 x 24 grid, drawn with a 2-unit round stroke (see Icon.qml). Most follow
// the Lucide icon set (ISC licence, https://lucide.dev); a few are drawn for this application.
// An entry is SVG path data, or { d, fill } for an icon that is filled as well.

function circle(cx, cy, r) {
    return "M" + (cx - r) + " " + cy
         + "a" + r + " " + r + " 0 1 0 " + (2 * r) + " 0"
         + "a" + r + " " + r + " 0 1 0 " + (-2 * r) + " 0z"
}

function rrect(x, y, w, h, r) {
    return "M" + (x + r) + " " + y + "h" + (w - 2 * r)
         + "a" + r + " " + r + " 0 0 1 " + r + " " + r + "v" + (h - 2 * r)
         + "a" + r + " " + r + " 0 0 1 " + (-r) + " " + r + "h" + (-(w - 2 * r))
         + "a" + r + " " + r + " 0 0 1 " + (-r) + " " + (-r) + "v" + (-(h - 2 * r))
         + "a" + r + " " + r + " 0 0 1 " + r + " " + (-r) + "z"
}

var icons = {
    activity: "M22 12h-4l-3 9L9 3l-3 9H2",
    alert: "M21.73 18l-8-14a2 2 0 0 0-3.48 0l-8 14A2 2 0 0 0 4 21h16a2 2 0 0 0 1.73-3zM12 9v4M12 17h.01",
    box: "M21 8a2 2 0 0 0-1-1.73l-7-4a2 2 0 0 0-2 0l-7 4A2 2 0 0 0 3 8v8a2 2 0 0 0 1 1.73l7 4a2 2 0 0 0 2 0l7-4A2 2 0 0 0 21 16zM3.3 7l8.7 5 8.7-5M12 22V12",
    check: "M20 6L9 17l-5-5",
    chevronDown: "M6 9l6 6 6-6",
    chevronLeft: "M15 18l-6-6 6-6",
    chevronRight: "M9 18l6-6-6-6",
    chevronUp: "M18 15l-6-6-6 6",
    clock: circle(12, 12, 10) + "M12 6v6l4 2",
    compass: circle(12, 12, 10) + "M16.24 7.76l-2.12 6.36-6.36 2.12 2.12-6.36z",
    cpu: rrect(4, 4, 16, 16, 2) + rrect(9, 9, 6, 6, 1)
         + "M15 2v2M15 20v2M2 15h2M2 9h2M20 15h2M20 9h2M9 2v2M9 20v2",
    crosshair: circle(12, 12, 10) + "M22 12h-4M6 12H2M12 6V2M12 22v-4",
    eraser: "M7 21l-4.3-4.3c-1-1-1-2.5 0-3.4l9.6-9.6c1-1 2.5-1 3.4 0l5.6 5.6c1 1 1 2.5 0 3.4L13 21M22 21H7M5 11l9 9",
    externalLink: "M15 3h6v6M10 14L21 3M18 13v6a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V8a2 2 0 0 1 2-2h6",
    flag: "M4 15s1-1 4-1 5 2 8 2 4-1 4-1V3s-1 1-4 1-5-2-8-2-4 1-4 1zM4 22v-7",
    folderOpen: "M6 14l1.5-2.9A2 2 0 0 1 9.24 10H20a2 2 0 0 1 1.94 2.5l-1.54 6a2 2 0 0 1-1.95 1.5H4a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h3.9a2 2 0 0 1 1.69.9l.81 1.2a2 2 0 0 0 1.67.9H18a2 2 0 0 1 2 2v2",
    gauge: "M12 14l4-4M3.34 19a10 10 0 1 1 17.32 0",
    grid: rrect(3, 3, 18, 18, 2) + "M3 9h18M3 15h18M9 3v18M15 3v18",
    info: circle(12, 12, 10) + "M12 16v-4M12 8h.01",
    keyboard: rrect(2, 4, 20, 16, 2) + "M6 8h.01M10 8h.01M14 8h.01M18 8h.01M8 12h.01M12 12h.01M16 12h.01M7 16h10",
    layers: "M12 2L2 7l10 5 10-5-10-5zM2 17l10 5 10-5M2 12l10 5 10-5",
    list: "M8 6h13M8 12h13M8 18h13M3 6h.01M3 12h.01M3 18h.01",
    locate: "M2 12h3M19 12h3M12 2v3M12 19v3" + circle(12, 12, 7) + circle(12, 12, 3),
    map: "M3 6l6-3 6 3 6-3v15l-6 3-6-3-6 3zM9 3v15M15 6v15",
    mapPin: "M20 10c0 6-8 12-8 12s-8-6-8-12a8 8 0 0 1 16 0z" + circle(12, 10, 3),
    maximize: "M15 3h6v6M9 21H3v-6M21 3l-7 7M3 21l7-7",
    minus: "M5 12h14",
    moon: "M12 3a6 6 0 0 0 9 9 9 9 0 1 1-9-9z",
    navigation: "M3 11l19-9-9 19-2-8-8-2z",
    network: rrect(9, 2, 6, 6, 1) + rrect(16, 16, 6, 6, 1) + rrect(2, 16, 6, 6, 1)
             + "M5 16v-3a1 1 0 0 1 1-1h12a1 1 0 0 1 1 1v3M12 12V8",
    panelBottom: rrect(3, 3, 18, 18, 2) + "M3 15h18",
    pause: { d: rrect(6, 4, 4, 16, 1) + rrect(14, 4, 4, 16, 1), fill: true },
    pencil: "M17 3a2.85 2.83 0 1 1 4 4L7.5 20.5 2 22l1.5-5.5zM15 5l4 4",
    play: { d: "M7 4.5v15l12-7.5z", fill: true },
    plus: "M5 12h14M12 5v14",
    power: "M12 2v10M18.4 6.6a9 9 0 1 1-12.77.04",
    radio: circle(12, 12, 2) + "M16.24 7.76a6 6 0 0 1 0 8.49M7.76 16.24a6 6 0 0 1 0-8.49"
           + "M19.07 4.93a10 10 0 0 1 0 14.14M4.93 19.07a10 10 0 0 1 0-14.14",
    refresh: "M3 12a9 9 0 0 1 9-9 9.75 9.75 0 0 1 6.74 2.74L21 8M21 3v5h-5"
             + "M21 12a9 9 0 0 1-9 9 9.75 9.75 0 0 1-6.74-2.74L3 16M8 16H3v5",
    rotateCcw: "M3 12a9 9 0 1 0 9-9 9.75 9.75 0 0 0-6.74 2.74L3 8M3 3v5h5",
    route: circle(6, 19, 3) + "M9 19h8.5a3.5 3.5 0 0 0 0-7h-11a3.5 3.5 0 0 1 0-7H15" + circle(18, 5, 3),
    save: "M15.2 3a2 2 0 0 1 1.4.6l3.8 3.8a2 2 0 0 1 .6 1.4V19a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2z"
          + "M17 21v-7a1 1 0 0 0-1-1H8a1 1 0 0 0-1 1v7M7 3v4a1 1 0 0 0 1 1h7",
    scan: "M3 7V5a2 2 0 0 1 2-2h2M17 3h2a2 2 0 0 1 2 2v2M21 17v2a2 2 0 0 1-2 2h-2M7 21H5a2 2 0 0 1-2-2v-2",
    sliders: "M21 4h-7M10 4H3M21 12h-9M8 12H3M21 20h-5M12 20H3M14 2v4M8 10v4M16 18v4",
    sprout: "M7 20h10M10 20c5.5-2.5.8-6.4 3-10"
            + "M9.5 9.4c1.1.8 1.8 2.2 2.3 3.7-2 .4-3.5.4-4.8-.3-1.2-.6-2.3-1.9-3-4.2 2.8-.5 4.4 0 5.5.8z"
            + "M14.1 6a7 7 0 0 0-1.1 4c1.9-.1 3.3-.6 4.3-1.4 1-1 1.6-2.3 1.7-4.6-2.7.1-4 1-4.9 2z",
    steering: circle(12, 12, 9) + circle(12, 12, 2.5) + "M9.5 12H3M14.5 12H21M12 14.5V21",
    stop: { d: rrect(5, 5, 14, 14, 2.5), fill: true },
    sun: circle(12, 12, 4) + "M12 2v2M12 20v2M4.93 4.93l1.41 1.41M17.66 17.66l1.41 1.41M2 12h2M20 12h2"
         + "M6.34 17.66l-1.41 1.41M19.07 4.93l-1.41 1.41",
    terminal: "M4 17l6-6-6-6M12 19h8",
    tractor: "M10 11l11 .9a1 1 0 0 1 .8 1.1l-.665 4.158a1 1 0 0 1-.988.842H20M16 18h-5"
             + "M18 5a1 1 0 0 0-1 1v5.573M3 4h8.129a1 1 0 0 1 .99.863L13 11.246M4 11V4M7 15h.01M8 10.1V4"
             + circle(18, 18, 2) + circle(7, 15, 5),
    trash: "M3 6h18M19 6v14c0 1-1 2-2 2H7c-1 0-2-1-2-2V6M8 6V4c0-1 1-2 2-2h4c1 0 2 1 2 2v2",
    upload: "M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4M17 8l-5-5-5 5M12 3v12",
    x: "M18 6L6 18M6 6l12 12",
    zap: "M13 2L3 14h9l-1 8 10-12h-9l1-8z"
}

function glyph(name) {
    const entry = icons[name]
    if (entry === undefined)
        return { d: "", fill: false }
    if (typeof entry === "string")
        return { d: entry, fill: false }
    return entry
}
