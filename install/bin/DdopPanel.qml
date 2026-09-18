import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

GroupBox {
    title: "Device descriptor (DDOP)"
    ListView {
        id: ddopList
        anchors.fill: parent
        model: ddopModel
        clip: true
        delegate: Text {
            width: ddopList.width
            leftPadding: 6 + indent * 18
            topPadding: 2
            bottomPadding: 2
            color: indent === 0 ? "#7fd0ff" : "#dfe6ee"
            font.pixelSize: indent === 0 ? 13 : 12
            font.bold: indent === 0
            wrapMode: Text.Wrap
            text: rowText
        }
    }
}
