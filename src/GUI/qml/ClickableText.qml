import QtQuick 2.4
import "."

Text {
    id: root
    signal clicked();

    property bool clickable: true
    property color baseTextColor: Style.baseTextColor
    color: mouse.containsMouse ? Style.themeColor : baseTextColor
    font.pixelSize: Style.baseFontPixelSize

    MouseArea {
        id: mouse

        enabled: root.clickable
        hoverEnabled: root.clickable
        anchors.fill: parent

        onClicked: parent.clicked();

        cursorShape: root.clickable ? Qt.PointingHandCursor : Qt.ArrowCursor
    }
}
