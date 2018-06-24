import QtQuick 2.4

Rectangle {
    id: root

    property alias text: buttonText.text
    property bool active: false

    signal clicked

    width: buttonText.width + (radius * 2)
    height: buttonText.height + (radius * 2)
    radius: 6

    color: mouse.containsMouse ? "#064989" : (active ? "#1b7ad3" : "white")

    StyledText {
        id: buttonText
        anchors.centerIn: parent
        color: (active | mouse.containsMouse) ? "white" : "#3f3f3f"
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
