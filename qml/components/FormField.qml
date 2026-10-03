import QtQuick
import QtQuick.Layouts
import AgIsoTc 1.0

// A form control with its label above and an optional hint below. The control is the child;
// give it Layout.fillWidth to span the field.
ColumnLayout {
    id: root

    property string label
    property string hint
    default property alias content: holder.data

    spacing: 6

    Text {
        visible: root.label !== ""
        text: root.label
        font.family: Theme.fontFamily
        font.pixelSize: Theme.fontSmall
        font.weight: Font.DemiBold
        color: Theme.textSecondary
    }

    ColumnLayout {
        id: holder
        Layout.fillWidth: true
        spacing: 6
    }

    Text {
        visible: root.hint !== ""
        Layout.fillWidth: true
        text: root.hint
        font.family: Theme.fontFamily
        font.pixelSize: Theme.fontCaption
        color: Theme.textMuted
        wrapMode: Text.Wrap
        lineHeight: 1.1
    }
}
