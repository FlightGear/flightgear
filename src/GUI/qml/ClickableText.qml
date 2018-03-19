import QtQuick 2.4
import "."

Text {
    signal clicked();

    property color baseTextColor: Style.baseTextColor
    color: mouse.containsMouse ? Style.themeColor : baseTextColor

    MouseArea {
        id: mouse
        hoverEnabled: true
        anchors.fill: parent

        onClicked: parent.clicked();

        cursorShape: Qt.PointingHandCursor
    }
}
