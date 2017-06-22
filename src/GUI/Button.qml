import QtQuick 2.0

Rectangle {
    id: root

    property string text
    property string hoverText: ""

    signal clicked

    width: 120
    height: 30
    radius: 6

    color: mouse.containsMouse ? "#064989" : "#1b7ad3"

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
