import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtCore
import AgIsoTc 1.0
import "components"
import "pages"

// The main window: header, navigation rail with its side panel, the 3D field view over the
// Task Controller data dock, and the status bar. Panel sizes, the open page and the theme are
// remembered between sessions.
ApplicationWindow {
    id: window

    visible: true
    width: 1600
    height: 960
    minimumWidth: 1180
    minimumHeight: 720
    title: "AgIso Task Controller Server"
    color: Theme.bg
    font.family: Theme.fontFamily
    font.pixelSize: Theme.fontBody

    palette.window: Theme.bg
    palette.windowText: Theme.text
    palette.base: Theme.inputBg
    palette.alternateBase: Theme.surfaceAlt
    palette.text: Theme.text
    palette.button: Theme.surfaceAlt
    palette.buttonText: Theme.text
    palette.highlight: Theme.accent
    palette.highlightedText: Theme.onAccent
    palette.toolTipBase: Theme.tooltipBg
    palette.toolTipText: Theme.tooltipText
    palette.placeholderText: Theme.textMuted
    palette.mid: Theme.borderStrong
    palette.dark: Theme.borderStrong
    palette.light: Theme.surfaceHover

    readonly property var pages: [
        { key: "connection", icon: "network", label: "Connect" },
        { key: "implements", icon: "box", label: "Implements" },
        { key: "gps", icon: "locate", label: "GPS" },
        { key: "field", icon: "map", label: "Fields" }
    ]
    property int page: 0
    property bool sidePanelOpen: true
    property bool dockOpen: true
    property real sidePanelWidth: 392
    property real dockHeight: 300
    readonly property int dockCollapsedHeight: 46

    function showPage(key) {
        for (let i = 0; i < pages.length; ++i) {
            if (pages[i].key === key) {
                page = i
                sidePanelOpen = true
                return
            }
        }
    }

    function openFieldMap() {
        mapWindow.visible = true
        mapWindow.requestActivate()
    }

    function openTaskData() {
        processDataWindow.visible = true
        processDataWindow.requestActivate()
    }

    Settings {
        category: "layout"
        property alias page: window.page
        property alias sidePanelOpen: window.sidePanelOpen
        property alias sidePanelWidth: window.sidePanelWidth
        property alias dockOpen: window.dockOpen
        property alias dockHeight: window.dockHeight
    }

    ServerConfig {
        id: serverConfig
    }

    Component.onCompleted: serverConfig.applyStartupOptions(startupOptions)

    // Drain the core event queue into the models.
    Timer {
        interval: 150
        running: true
        repeat: true
        onTriggered: bridge.poll()
    }

    Connections {
        target: bridge
        function onIdentifyBanner(tcNumber) {
            toast.show("TC " + tcNumber + " — identify requested")
        }
    }

    // counts the TC clients for the rail badge and the status bar
    Instantiator {
        id: clientCounter
        model: clientModel
        delegate: QtObject { }
    }

    MapWindow {
        id: mapWindow
    }

    ProcessDataWindow {
        id: processDataWindow
    }

    component PaneHandle: Item {
        id: handle
        readonly property bool horizontal: parent ? parent.orientation === Qt.Horizontal : true
        implicitWidth: 8
        implicitHeight: 8
        Rectangle {
            anchors.centerIn: parent
            width: handle.horizontal ? 3 : 40
            height: handle.horizontal ? 40 : 3
            radius: 1.5
            color: handle.SplitHandle.pressed ? Theme.accent
                                              : (handle.SplitHandle.hovered ? Theme.borderStrong : Theme.alpha(Theme.borderStrong, 0))
            Behavior on color { ColorAnimation { duration: Theme.fast } }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        AppHeader {
            Layout.fillWidth: true
            config: serverConfig
            onConnectionClicked: window.showPage("connection")
            onTaskClicked: window.showPage("field")
            onOpenFieldMapRequested: window.openFieldMap()
            onOpenTaskDataRequested: window.openTaskData()
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            NavRail {
                Layout.fillHeight: true
                pages: window.pages
                currentIndex: window.page
                panelOpen: window.sidePanelOpen
                indicators: [
                    { tone: bridge.running ? "success" : "" },
                    { count: clientCounter.count },
                    { tone: bridge.gpsValid ? "success" : (bridge.gpsRunning ? "warning" : "") },
                    { tone: bridge.taskActive ? "danger" : "" }
                ]
                onActivated: function(index) {
                    if (index === window.page && window.sidePanelOpen) {
                        window.sidePanelOpen = false
                    } else {
                        window.page = index
                        window.sidePanelOpen = true
                    }
                }
            }

            SplitView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.rightMargin: 10
                orientation: Qt.Horizontal
                handle: PaneHandle { }

                Rectangle {
                    id: sidePanel
                    visible: window.sidePanelOpen
                    SplitView.preferredWidth: window.sidePanelWidth
                    SplitView.minimumWidth: 330
                    SplitView.maximumWidth: 620
                    onWidthChanged: if (visible && width > 0) window.sidePanelWidth = width
                    radius: Theme.radiusLg
                    color: Theme.surface
                    border.color: Theme.border

                    StackLayout {
                        anchors.fill: parent
                        anchors.margins: 1
                        currentIndex: window.page

                        ConnectionPage {
                            config: serverConfig
                        }
                        ImplementsPage {
                            onShowPageRequested: function(key) { window.showPage(key) }
                        }
                        GpsPage { }
                        FieldTaskPage {
                            onOpenFieldMapRequested: window.openFieldMap()
                            onShowPageRequested: function(key) { window.showPage(key) }
                        }
                    }
                }

                SplitView {
                    SplitView.fillWidth: true
                    SplitView.minimumWidth: 480
                    orientation: Qt.Vertical
                    handle: PaneHandle { }

                    SectionView3D {
                        SplitView.fillHeight: true
                        SplitView.minimumHeight: 260
                        onOpenFieldMapRequested: window.openFieldMap()
                        onShowPageRequested: function(key) { window.showPage(key) }
                    }

                    ProcessDataPanel {
                        id: dock
                        collapsed: !window.dockOpen
                        SplitView.minimumHeight: window.dockCollapsedHeight
                        SplitView.maximumHeight: collapsed ? window.dockCollapsedHeight : 100000
                        // SplitView writes preferredHeight while the handle is dragged, so it is
                        // set here rather than bound.
                        function applyHeight() {
                            dock.SplitView.preferredHeight = collapsed ? window.dockCollapsedHeight : window.dockHeight
                        }
                        Component.onCompleted: applyHeight()
                        onCollapsedChanged: applyHeight()
                        onHeightChanged: if (!collapsed && height > window.dockCollapsedHeight + 40) window.dockHeight = height
                        onCollapseToggled: window.dockOpen = !window.dockOpen
                        onPopOutRequested: window.openTaskData()
                    }
                }
            }
        }

        StatusBar {
            Layout.fillWidth: true
            clientCount: clientCounter.count
        }
    }

    Toast {
        id: toast
        iconName: "radio"
        tone: "info"
        topOffset: 70
    }
}
