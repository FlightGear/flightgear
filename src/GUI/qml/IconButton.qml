import QtQuick 2.0
import "."

Rectangle {
    id: root
    radius: Style.roundRadius
    border.width: 1
    border.color: Style.themeColor
    width: height
    height: Style.baseFontPixelSize + Style.margin * 2
    color: mouse.containsMouse ? Style.minorFrameColor : "white"

    property alias icon: icon.source

    signal clicked();

    Image {
        id:  icon
        width: parent.width - Style.margin
        height: parent.height - Style.margin
        anchors.centerIn: parent
    }

    MouseArea {
        id: mouse
        hoverEnabled: true
        onClicked: root.clicked();
        anchors.fill: parent
    }
}
