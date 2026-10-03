import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import AgIsoTc 1.0

// A vertically scrolling page: children are laid out in a padded column.
Flickable {
    id: root

    property int padding: Theme.s4
    property alias spacing: column.spacing
    default property alias content: column.data

    clip: true
    contentWidth: width
    contentHeight: column.implicitHeight + 2 * padding
    boundsBehavior: Flickable.StopAtBounds
    ScrollBar.vertical: AppScrollBar { }

    ColumnLayout {
        id: column
        x: root.padding
        y: root.padding
        width: root.width - 2 * root.padding
        spacing: Theme.s4
    }
}
