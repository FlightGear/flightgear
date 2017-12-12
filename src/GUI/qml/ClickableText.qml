import QtQuick 2.0
import "."

Text {
    signal clicked();

    color: mouse.containsMouse ? Style.themeColor : Style.baseTextColor

    MouseArea {
        id: mouse
        hoverEnabled: true
        anchors.fill: parent

        onClicked: parent.clicked();
    }
}
