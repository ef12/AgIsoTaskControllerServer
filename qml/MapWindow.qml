import QtQuick
import QtQuick.Layouts
import AgIsoTc 1.0
import "components"

// Detached field-operation window: the 2D map with floating panels for the GPS position, the
// field drawing tools and the sections.
Window {
    id: root
    visible: false
    width: 1200
    height: 800
    minimumWidth: 860
    minimumHeight: 600
    title: "Field map"
    color: Theme.bg

    Item {
        anchors.fill: parent
        anchors.margins: 10

        FieldMapView {
            id: fieldMap
            anchors.fill: parent
            drawMode: drawSwitch.checked
            nudgeStepM: Number(nudgeStepBox.currentText)
        }

        CornerMask {
            radius: Theme.radiusLg
            color: Theme.bg
        }
        Rectangle {
            anchors.fill: parent
            radius: Theme.radiusLg
            color: "transparent"
            border.color: Theme.border
        }

        // --- GPS -------------------------------------------------------------------------------
        GlassPanel {
            x: 12
            y: 12
            width: gpsRow.implicitWidth + 24
            height: 48

            RowLayout {
                id: gpsRow
                anchors.centerIn: parent
                spacing: 10
                CompassDial {
                    size: 30
                    course: bridge.gpsCourse
                    active: bridge.gpsValid
                }
                ColumnLayout {
                    spacing: 0
                    Text {
                        text: bridge.gpsValid ? bridge.gpsLatitude.toFixed(7) + ", " + bridge.gpsLongitude.toFixed(7)
                                              : "Waiting for a position"
                        font.family: bridge.gpsValid ? Theme.monoFamily : Theme.fontFamily
                        font.pixelSize: Theme.fontSmall
                        font.weight: Font.DemiBold
                        color: bridge.gpsValid ? Theme.text : Theme.textMuted
                    }
                    Text {
                        text: bridge.gpsValid ? bridge.gpsSpeedKph.toFixed(1) + " km/h · " + bridge.gpsCourse.toFixed(0) + "°"
                                              : "GPS: " + bridge.gpsSourceText
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontCaption
                        color: Theme.textMuted
                    }
                }
            }
        }

        // --- drawing tools ---------------------------------------------------------------------
        GlassPanel {
            anchors.horizontalCenter: parent.horizontalCenter
            y: 12
            width: drawRow.implicitWidth + 20
            height: 52

            RowLayout {
                id: drawRow
                anchors.centerIn: parent
                spacing: 10

                AppSwitch {
                    id: drawSwitch
                    text: "Draw"
                    checked: true
                    focusPolicy: Qt.NoFocus
                }
                Rectangle {
                    Layout.preferredWidth: 1
                    Layout.preferredHeight: 22
                    color: Theme.hudBorder
                }
                AppTextField {
                    id: fieldNameField
                    Layout.preferredWidth: 160
                    placeholderText: "Field name"
                    text: "Field " + (bridge.fieldNames.length + 1)
                }
                Text {
                    text: "Nudge"
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSmall
                    color: Theme.textMuted
                }
                AppComboBox {
                    id: nudgeStepBox
                    Layout.preferredWidth: 92
                    model: ["0.01", "0.05", "0.1", "0.5", "1", "5"]
                    currentIndex: 2
                    displayText: currentText + " m"
                    focusPolicy: Qt.NoFocus
                }
                AppButton {
                    text: "Clear"
                    iconName: "x"
                    variant: "ghost"
                    enabled: fieldMap.draftPoints.length > 0
                    focusPolicy: Qt.NoFocus
                    onClicked: fieldMap.clearDraft()
                }
                AppButton {
                    text: "Create field"
                    iconName: "check"
                    variant: "primary"
                    enabled: fieldMap.draftPoints.length >= 3
                    focusPolicy: Qt.NoFocus
                    onClicked: {
                        if (bridge.createFieldFromLocalBoundary(fieldNameField.text, fieldMap.draftPoints))
                            fieldMap.clearDraft()
                    }
                }
            }
        }

        // --- a rate zone of the selected task's prescription, from the drawn polygon ------------
        GlassPanel {
            id: zonePanel
            visible: drawSwitch.checked && bridge.selectedTaskIndex >= 0
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.rightMargin: 68
            anchors.topMargin: 12
            width: zoneRow.implicitWidth + 20
            height: 52

            // the rates the client can take: "Name (DDI n)"
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

            RowLayout {
                id: zoneRow
                anchors.centerIn: parent
                spacing: 8
                AppComboBox {
                    id: zoneRate
                    visible: zonePanel.clientRates.length > 0
                    Layout.preferredWidth: 190
                    model: zonePanel.clientRates.map(function(rate) { return rate.text })
                    focusPolicy: Qt.NoFocus
                }
                AppSpinBox {
                    id: zoneDdi
                    visible: zonePanel.clientRates.length === 0
                    Layout.preferredWidth: 110
                    from: 1
                    to: 65535
                    value: 6
                }
                AppSpinBox {
                    id: zoneValue
                    Layout.preferredWidth: 130
                    from: 0
                    to: 2000000000
                    stepSize: 1000
                    value: 15000
                }
                AppButton {
                    text: "Add rate zone"
                    iconName: "layers"
                    enabled: fieldMap.draftPoints.length >= 3
                    focusPolicy: Qt.NoFocus
                    tip: "Adds the drawn polygon as a treatment zone with this rate (raw DDOP unit) to the selected task's prescription"
                    onClicked: {
                        const rates = zonePanel.clientRates
                        const ddi = rates.length > 0 && zoneRate.currentIndex >= 0 ? rates[zoneRate.currentIndex].ddi : zoneDdi.value
                        if (bridge.addRateZone(fieldMap.draftPoints, ddi, zoneValue.value))
                            fieldMap.clearDraft()
                    }
                }
            }
        }

        // drawing help
        GlassPanel {
            visible: drawSwitch.checked
            anchors.left: parent.left
            anchors.bottom: sections.top
            anchors.margins: 12
            width: helpColumn.implicitWidth + 24
            height: helpColumn.implicitHeight + 20

            ColumnLayout {
                id: helpColumn
                anchors.centerIn: parent
                spacing: 4
                Text {
                    text: fieldMap.draftPoints.length === 0 ? "Click on the map to place the first corner"
                          : fieldMap.draftPoints.length + (fieldMap.draftPoints.length === 1 ? " corner" : " corners")
                            + (fieldMap.draftPoints.length < 3 ? " — at least 3 make a field" : "")
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSmall
                    font.weight: Font.DemiBold
                    color: Theme.text
                }
                Text {
                    text: "Drag a corner to move it · arrow keys nudge the selected one · Delete removes it"
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontCaption
                    color: Theme.textMuted
                }
            }
        }

        // --- sections --------------------------------------------------------------------------
        SectionStatusPanel {
            id: sections
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.leftMargin: 12
            anchors.rightMargin: 12
            anchors.bottomMargin: 12
            height: implicitHeight
        }
    }
}
