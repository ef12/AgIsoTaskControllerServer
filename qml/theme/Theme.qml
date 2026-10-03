pragma Singleton
import QtQuick
import QtCore

// Design tokens of the whole GUI: colours for a dark and a light theme, type scale, spacing,
// radii and motion. Registered from C++ as the singleton AgIsoTc.Theme, so every file reads
// the same values (`import AgIsoTc 1.0`, then Theme.surface). The chosen theme is remembered.
// The accent green comes from the application logo.
QtObject {
    id: theme

    property bool dark: true

    readonly property Settings _settings: Settings {
        category: "appearance"
        property alias darkTheme: theme.dark
    }

    // --- surfaces -----------------------------------------------------------------------------
    readonly property color bg: dark ? "#0c0e11" : "#eef0f3"
    readonly property color surface: dark ? "#14171b" : "#ffffff"
    readonly property color surfaceAlt: dark ? "#1a1e23" : "#f6f7f9"
    readonly property color surfaceHover: dark ? "#20252b" : "#eef0f3"
    readonly property color surfacePressed: dark ? "#272d34" : "#e4e7eb"
    readonly property color inputBg: dark ? "#0f1215" : "#ffffff"
    readonly property color popupBg: dark ? "#1b1f25" : "#ffffff"
    readonly property color border: dark ? "#23282f" : "#e2e5e9"
    readonly property color borderStrong: dark ? "#323943" : "#cdd2d9"
    readonly property color divider: dark ? "#1e2329" : "#eceef1"

    // --- text -------------------------------------------------------------------------------
    readonly property color text: dark ? "#e7eaee" : "#14181f"
    readonly property color textSecondary: dark ? "#a2abb6" : "#4d5562"
    readonly property color textMuted: dark ? "#6c7682" : "#7d8591"
    readonly property color textDisabled: dark ? "#47505a" : "#b6bcc5"

    // --- accent and status ------------------------------------------------------------------
    readonly property color accent: dark ? "#43c26a" : "#1f8a3d"
    readonly property color accentHover: dark ? "#56cf7c" : "#1a7a35"
    readonly property color accentPressed: dark ? "#37a95b" : "#156a2d"
    readonly property color accentText: dark ? "#5fd685" : "#18803a"
    readonly property color onAccent: dark ? "#04130a" : "#ffffff"
    readonly property color success: accent
    readonly property color warning: dark ? "#f2b544" : "#b26f12"
    readonly property color danger: dark ? "#f0625f" : "#d63c39"
    readonly property color info: dark ? "#5ea8ff" : "#2563eb"
    readonly property color violet: dark ? "#b29bff" : "#7c3aed"
    readonly property color neutral: textMuted

    // --- overlays ---------------------------------------------------------------------------
    readonly property color tooltipBg: dark ? "#2a3038" : "#1f242b"
    readonly property color tooltipText: "#f1f3f6"
    readonly property color hudBg: dark ? Qt.rgba(0.075, 0.085, 0.10, 0.82) : Qt.rgba(1, 1, 1, 0.88)
    readonly property color hudBorder: dark ? Qt.rgba(1, 1, 1, 0.08) : Qt.rgba(0, 0, 0, 0.07)
    readonly property color shadow: dark ? Qt.rgba(0, 0, 0, 0.55) : Qt.rgba(0.06, 0.09, 0.14, 0.18)
    readonly property color scrollThumb: dark ? Qt.rgba(1, 1, 1, 0.16) : Qt.rgba(0, 0, 0, 0.18)
    readonly property color scrollThumbHover: dark ? Qt.rgba(1, 1, 1, 0.28) : Qt.rgba(0, 0, 0, 0.30)
    readonly property color selection: alpha(accent, dark ? 0.35 : 0.25)

    // --- section LEDs -----------------------------------------------------------------------
    readonly property color ledOn: dark ? "#3ddc6e" : "#22b455"
    readonly property color ledOnBorder: dark ? "#a7f3c0" : "#15803d"
    readonly property color ledOnText: dark ? "#052e12" : "#ffffff"
    readonly property color ledOff: dark ? "#20262d" : "#e3e7eb"
    readonly property color ledOffBorder: dark ? "#333b45" : "#c9cfd6"
    readonly property color ledRail: dark ? "#101317" : "#f1f3f5"

    // --- 3D scene ---------------------------------------------------------------------------
    readonly property color skyTop: dark ? "#070b11" : "#7fb2e0"
    readonly property color skyHorizon: dark ? "#1f2b37" : "#dfe9f1"
    readonly property color groundFar: dark ? "#11171c" : "#c9d1c3"
    readonly property color ground: dark ? "#232d34" : "#c3ccb9"
    readonly property color fieldIdle: dark ? "#2a363f" : "#b7c2ab"
    readonly property color fieldActive: dark ? "#2b5234" : "#9fc286"
    readonly property color grid10: dark ? "#33414c" : "#aab4a0"
    readonly property color grid50: dark ? "#4d6273" : "#8a9881"
    readonly property color coverage: dark ? "#69b83d" : "#4f9a2a"
    readonly property color boundaryMarker: dark ? "#e6edf3" : "#1d2733"
    readonly property color recording: "#ff9f0a"

    // --- 2D field map -----------------------------------------------------------------------
    readonly property color mapBg: dark ? "#0e1215" : "#f4f6f8"
    readonly property color mapGrid: dark ? "#192026" : "#e5e9ed"
    readonly property color mapGridMajor: dark ? "#25303a" : "#d0d7de"

    // --- data traffic directions ------------------------------------------------------------
    readonly property color fromTc: info
    readonly property color fromClient: warning

    // --- typography -------------------------------------------------------------------------
    readonly property string fontFamily: pickFont(["Segoe UI", "Inter", "SF Pro Text", "Helvetica Neue",
                                                    "Noto Sans", "Ubuntu", "Cantarell"])
    readonly property string monoFamily: pickFont(["Cascadia Mono", "Cascadia Code", "JetBrains Mono",
                                                    "Consolas", "SF Mono", "Menlo", "DejaVu Sans Mono",
                                                    "Courier New"])
    readonly property int fontCaption: 11
    readonly property int fontSmall: 12
    readonly property int fontBody: 13
    readonly property int fontTitle: 14
    readonly property int fontHeading: 18
    readonly property int fontDisplay: 24

    // --- spacing, shape, motion -------------------------------------------------------------
    readonly property int s1: 4
    readonly property int s2: 8
    readonly property int s3: 12
    readonly property int s4: 16
    readonly property int s5: 20
    readonly property int s6: 24
    readonly property int radiusSm: 6
    readonly property int radiusMd: 8
    readonly property int radiusLg: 12
    readonly property int controlHeight: 32
    readonly property int controlHeightSm: 28
    readonly property int controlHeightLg: 38
    readonly property int fast: 110
    readonly property int normal: 180
    readonly property int slow: 280

    function alpha(colour, value) {
        return Qt.rgba(colour.r, colour.g, colour.b, value)
    }

    function tone(name) {
        switch (name) {
        case "accent": return accent
        case "success": return success
        case "warning": return warning
        case "danger": return danger
        case "info": return info
        default: return textMuted
        }
    }

    function pickFont(candidates) {
        const available = Qt.fontFamilies()
        for (let i = 0; i < candidates.length; ++i) {
            if (available.indexOf(candidates[i]) >= 0)
                return candidates[i]
        }
        return Qt.application.font.family
    }
}
