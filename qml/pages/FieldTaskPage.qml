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

    // --- prescription (TC-GEO variable rate) -----------------------------------------------------
    Card {
        id: prescriptionCard
        Layout.fillWidth: true
        title: "Prescription"
        subtitle: bridge.prescription.present ? bridge.prescription.name
                                              : (page.taskSelected ? "No map for this task" : "Select a task first")
        iconName: "layers"
        actions: [
            AppButton {
                variant: "ghost"
                size: "sm"
                iconName: "upload"
                tip: "Import ISO 11783-10 task data (TASKDATA.XML with its grid files): its tasks, prescriptions and fields"
                onClicked: importTaskDataDialog.open()
            },
            AppButton {
                variant: "ghost"
                size: "sm"
                iconName: "trash"
                enabled: bridge.prescription.present === true
                tip: "Remove the task's map"
                onClicked: bridge.clearPrescription()
            }
        ]

        // the rates the client can take, for the test map: "Name (DDI n)"
        readonly property var clientRates: {
            const rates = []
            const seen = {}
            for (let c = 0; c < bridge.rateChannels.length; ++c) {
                const groups = bridge.rateChannels[c].groups
                for (let g = 0; g < groups.length; ++g) {
                    if (seen[groups[g].ddi]) continue
                    seen[groups[g].ddi] = true
                    rates.push({ "ddi": groups[g].ddi, "text": groups[g].name + " (DDI " + groups[g].ddi + ")" })
                }
            }
            return rates
        }

        ColumnLayout {
            visible: bridge.prescription.present === true
            Layout.fillWidth: true
            spacing: 8

            Text {
                Layout.fillWidth: true
                text: bridge.prescription.description || ""
                wrapMode: Text.Wrap
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSmall
                color: Theme.textSecondary
            }
            FormField {
                Layout.fillWidth: true
                label: "Layer shown on the map"
                AppComboBox {
                    Layout.fillWidth: true
                    model: (bridge.prescription.layers || []).map(function(layer) { return layer.name })
                    currentIndex: bridge.prescription.selectedLayer !== undefined ? bridge.prescription.selectedLayer : -1
                    onActivated: function(index) { bridge.selectPrescriptionLayer(index) }
                }
            }
            // legend: the colour scale from the lowest to the highest rate of the layer
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 10
                    radius: 3
                    gradient: Gradient {
                        orientation: Gradient.Horizontal
                        GradientStop { position: 0.0; color: "#d7191c" }
                        GradientStop { position: 0.25; color: "#fdae61" }
                        GradientStop { position: 0.5; color: "#ffffbf" }
                        GradientStop { position: 0.75; color: "#a6d96a" }
                        GradientStop { position: 1.0; color: "#1a9641" }
                    }
                }
                RowLayout {
                    Layout.fillWidth: true
                    Text {
                        text: (bridge.prescription.legendMinimum || "") + " " + (bridge.prescription.legendUnit || "")
                        font.family: Theme.monoFamily
                        font.pixelSize: Theme.fontCaption
                        color: Theme.textMuted
                    }
                    Item { Layout.fillWidth: true }
                    Text {
                        text: (bridge.prescription.legendMaximum || "") + " " + (bridge.prescription.legendUnit || "")
                        font.family: Theme.monoFamily
                        font.pixelSize: Theme.fontCaption
                        color: Theme.textMuted
                    }
                }
            }
        }

        Divider { Layout.fillWidth: true }

        SectionLabel { text: "Test map" }

        FormField {
            Layout.fillWidth: true
            label: "Rate"
            hint: prescriptionCard.clientRates.length === 0 ? "No client rates: enter the DDI" : ""
            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                AppComboBox {
                    id: testRate
                    visible: prescriptionCard.clientRates.length > 0
                    Layout.fillWidth: true
                    model: prescriptionCard.clientRates.map(function(rate) { return rate.text })
                }
                AppSpinBox {
                    id: testDdi
                    visible: prescriptionCard.clientRates.length === 0
                    Layout.fillWidth: true
                    from: 1
                    to: 65535
                    value: 6
                }
            }
        }
        RowLayout {
            Layout.fillWidth: true
            spacing: 12
            FormField {
                Layout.fillWidth: true
                label: "Pattern"
                AppComboBox {
                    id: testPattern
                    Layout.fillWidth: true
                    model: ["Checkerboard", "Stripes (across the implement)", "Bands (along the track)", "Gradient (west to east)"]
                }
            }
        }
        RowLayout {
            Layout.fillWidth: true
            spacing: 12
            FormField {
                Layout.fillWidth: true
                label: "Rate A (raw)"
                AppSpinBox {
                    id: testRateA
                    Layout.fillWidth: true
                    from: 0
                    to: 2000000000
                    stepSize: 1000
                    value: 10000
                }
            }
            FormField {
                Layout.fillWidth: true
                label: "Rate B (raw)"
                AppSpinBox {
                    id: testRateB
                    Layout.fillWidth: true
                    from: 0
                    to: 2000000000
                    stepSize: 1000
                    value: 20000
                }
            }
        }
        RowLayout {
            Layout.fillWidth: true
            spacing: 12
            FormField {
                Layout.fillWidth: true
                label: "Cell (m)"
                AppSpinBox {
                    id: testCell
                    Layout.fillWidth: true
                    from: 1
                    to: 100
                    value: 5
                }
            }
            FormField {
                Layout.fillWidth: true
                label: "Pattern size (m)"
                AppSpinBox {
                    id: testSize
                    Layout.fillWidth: true
                    from: 1
                    to: 1000
                    value: 12
                }
            }
        }
        AppButton {
            Layout.fillWidth: true
            text: "Add to the task's map"
            iconName: "plus"
            enabled: page.taskSelected
            onClicked: {
                const ddi = prescriptionCard.clientRates.length > 0 && testRate.currentIndex >= 0
                          ? prescriptionCard.clientRates[testRate.currentIndex].ddi : testDdi.value
                bridge.createTestPrescription(ddi, testPattern.currentIndex, testRateA.value, testRateB.value,
                                              testCell.value, testSize.value)
            }
        }
        Text {
            Layout.fillWidth: true
            text: "A grid over the task's field, in the DDOP's raw unit of the rate. While the task runs, the TC-GEO tab sends each control channel the map's rate where its elements are."
            wrapMode: Text.Wrap
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontCaption
            color: Theme.textMuted
        }
    }

    FileDialog {
        id: importTaskDataDialog
        title: "Import ISO 11783-10 task data"
        nameFilters: ["ISOXML task data (TASKDATA.XML *.xml *.XML)", "All files (*)"]
        onAccepted: bridge.importTaskData(selectedFile)
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
