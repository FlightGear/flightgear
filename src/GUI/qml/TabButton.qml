import QtQuick 2.4
import FlightGear 1.0

Rectangle {
    id: root

    property alias text: buttonText.text
    property bool active: false

    signal clicked

    width: buttonText.width + (radius * 2)
    height: buttonText.height + (radius * 2)
    radius: 6

    color: mouse.containsMouse ? Style.activeColor : (active ? Style.themeColor : Style.backgroundColor)

    StyledText {
        id: buttonText
        anchors.centerIn: parent
        color: (active | mouse.containsMouse) ? Style.themeContrastTextColor : Style.baseTextColor
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
