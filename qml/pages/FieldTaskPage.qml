import QtQuick
import QtQuick.Dialogs
import QtQuick.Layouts
import AgIsoTc 1.0
import "../components"

// Fields (select, create at the GPS position or draw on the map, save and load) and tasks
// (select, create for the field, start, pause and complete, save and load).
ScrollPage {
    id: page

    signal openFieldMapRequested()
    signal showPageRequested(string key)

    readonly property bool fieldSelected: bridge.selectedFieldIndex >= 0
    readonly property bool taskSelected: bridge.selectedTaskIndex >= 0

    PageHeader {
        Layout.fillWidth: true
        title: "Field & task"
        subtitle: "Where the implement works and what is recorded"
    }

    // --- field -------------------------------------------------------------------------------
    Card {
        Layout.fillWidth: true
        title: "Field"
        subtitle: bridge.fieldNames.length === 0 ? "No field yet" : bridge.fieldNames.length
                                                                    + (bridge.fieldNames.length === 1 ? " field" : " fields")
        iconName: "map"
        actions: [
            AppButton {
                variant: "ghost"
                size: "sm"
                iconName: "save"
                enabled: bridge.fieldNames.length > 0
                tip: "Save the fields to a file"
                onClicked: saveFieldsDialog.open()
            },
            AppButton {
                variant: "ghost"
                size: "sm"
                iconName: "folderOpen"
                tip: "Load fields from a file"
                onClicked: loadFieldsDialog.open()
            }
        ]

        AppComboBox {
            Layout.fillWidth: true
            model: bridge.fieldNames
            currentIndex: bridge.selectedFieldIndex
            displayText: count > 0 && currentIndex >= 0 ? currentText : "No field defined"
            onActivated: bridge.selectField(currentIndex)
        }

        RowLayout {
            visible: page.fieldSelected
            Layout.fillWidth: true
            spacing: 0
            Metric {
                Layout.fillWidth: true
                label: "Size"
                value: bridge.fieldWidthM.toFixed(0) + " × " + bridge.fieldLengthM.toFixed(0)
                unit: "m"
                valueSize: 15
            }
            Metric {
                Layout.fillWidth: true
                label: "Boundary"
                value: bridge.boundaryPointCount > 0 ? bridge.boundaryPointCount : "—"
                unit: bridge.boundaryPointCount > 0 ? "points" : ""
                valueSize: 15
            }
        }

        Divider { Layout.fillWidth: true }

        SectionLabel { text: "New field" }

        FormField {
            Layout.fillWidth: true
            label: "Name"
            AppTextField {
                id: fieldName
                Layout.fillWidth: true
                text: "Field " + (bridge.fieldNames.length + 1)
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 12
            FormField {
                Layout.fillWidth: true
                label: "Width (m)"
                AppSpinBox {
                    id: fieldWidth
                    Layout.fillWidth: true
                    from: 1
                    to: 10000
                    value: 200
                }
            }
            FormField {
                Layout.fillWidth: true
                label: "Length (m)"
                AppSpinBox {
                    id: fieldLength
                    Layout.fillWidth: true
                    from: 1
                    to: 10000
                    value: 300
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            AppButton {
                Layout.fillWidth: true
                text: "Create at GPS position"
                iconName: "mapPin"
                enabled: bridge.gpsValid
                onClicked: bridge.createField(fieldName.text, fieldWidth.value, fieldLength.value)
            }
            AppButton {
                text: "Draw"
                iconName: "pencil"
                tip: "Draw the field boundary on the field map"
                onClicked: page.openFieldMapRequested()
            }
        }

        Text {
            visible: !bridge.gpsValid
            Layout.fillWidth: true
            text: "A field is created around the GPS position, so start the GPS first — or draw one on the map."
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontCaption
            color: Theme.textMuted
            wrapMode: Text.Wrap
        }
    }

    // --- task --------------------------------------------------------------------------------
    Card {
        Layout.fillWidth: true
        title: "Task"
        subtitle: bridge.taskActive ? "Recording " + bridge.activeTaskName + " on " + bridge.activeFieldName
                                    : (page.taskSelected ? "Ready to start" : "No task selected")
        iconName: "flag"
        iconColor: bridge.taskActive ? Theme.danger : Theme.accentText
        actions: [
            AppButton {
                variant: "ghost"
                size: "sm"
                iconName: "save"
                enabled: bridge.taskNames.length > 0
                tip: "Save the tasks to a file"
                onClicked: saveTasksDialog.open()
            },
            AppButton {
                variant: "ghost"
                size: "sm"
                iconName: "folderOpen"
                tip: "Load tasks from a file. Tasks keep their field link, so load the fields first."
                onClicked: loadTasksDialog.open()
            }
        ]

        AppComboBox {
            Layout.fillWidth: true
            model: bridge.taskNames
            currentIndex: bridge.selectedTaskIndex
            displayText: count > 0 && currentIndex >= 0 ? currentText : "No task defined"
            onActivated: bridge.selectTask(currentIndex)
        }

        // recording state
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 44
            radius: Theme.radiusMd
            color: bridge.taskActive ? Theme.alpha(Theme.danger, Theme.dark ? 0.1 : 0.07) : Theme.surface
            border.color: bridge.taskActive ? Theme.alpha(Theme.danger, 0.35) : Theme.border

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                spacing: 10
                PulseDot {
                    color: bridge.taskActive ? Theme.danger : Theme.textMuted
                    pulse: bridge.taskActive
                }
                Text {
                    Layout.fillWidth: true
                    text: bridge.taskActive ? "Recording  ·  " + bridge.activeTaskName + " / " + bridge.activeFieldName
                                            : "Not recording"
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSmall
                    font.weight: Font.DemiBold
                    color: bridge.taskActive ? Theme.text : Theme.textMuted
                    elide: Text.ElideRight
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            AppButton {
                Layout.fillWidth: true
                text: "Start"
                iconName: "play"
                variant: "primary"
                enabled: page.taskSelected && !bridge.taskActive
                onClicked: bridge.startSelectedTask()
            }
            AppButton {
                Layout.fillWidth: true
                text: "Pause"
                iconName: "pause"
                enabled: bridge.taskActive
                onClicked: bridge.pauseSelectedTask()
            }
            AppButton {
                Layout.fillWidth: true
                text: "Complete"
                iconName: "check"
                enabled: page.taskSelected
                onClicked: bridge.stopSelectedTask()
            }
        }

        AppButton {
            Layout.alignment: Qt.AlignLeft
            variant: "ghost"
            size: "sm"
            text: "Clear track"
            iconName: "eraser"
            onClicked: bridge.clearTrack()
        }

        Divider { Layout.fillWidth: true }

        SectionLabel { text: "New task" }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            AppTextField {
                id: taskName
                Layout.fillWidth: true
                text: "Task " + (bridge.taskNames.length + 1)
            }
            AppButton {
                text: "Create"
                iconName: "plus"
                enabled: page.fieldSelected
                onClicked: bridge.createTask(taskName.text)
            }
        }

        Text {
            visible: !page.fieldSelected
            Layout.fillWidth: true
            text: "A task belongs to a field: select or create a field first."
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontCaption
            color: Theme.textMuted
            wrapMode: Text.Wrap
        }
    }

    FileDialog {
        id: saveFieldsDialog
        title: "Save fields"
        fileMode: FileDialog.SaveFile
        defaultSuffix: "json"
        nameFilters: ["Field files (*.json)", "All files (*)"]
        onAccepted: bridge.saveFields(selectedFile)
    }
    FileDialog {
        id: loadFieldsDialog
        title: "Load fields"
        nameFilters: ["Field files (*.json)", "All files (*)"]
        onAccepted: bridge.loadFields(selectedFile)
    }
    FileDialog {
        id: saveTasksDialog
        title: "Save tasks"
        fileMode: FileDialog.SaveFile
        defaultSuffix: "json"
        nameFilters: ["Task files (*.json)", "All files (*)"]
        onAccepted: bridge.saveTasks(selectedFile)
    }
    FileDialog {
        id: loadTasksDialog
        title: "Load tasks"
        nameFilters: ["Task files (*.json)", "All files (*)"]
        onAccepted: bridge.loadTasks(selectedFile)
    }
}
