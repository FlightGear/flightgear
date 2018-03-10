import QtQuick 2.0
import "."

Rectangle {
    id: root

    property string text
    property string hoverText: ""

    signal clicked

    width: Style.strutSize * 2
    height: buttonText.implicitHeight + (radius * 2)
    radius: Style.roundRadius

    color: mouse.containsMouse ? Style.activeColor : Style.themeColor

    Text {
        id: buttonText
        anchors.centerIn: parent
        color: "white"
        text: (mouse.containsMouse && hoverText != "") ? root.hoverText : root.text
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true

        onClicked: {
            root.clicked();
        }

    }
}
