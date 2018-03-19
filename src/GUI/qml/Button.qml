import QtQuick 2.4
import "."

Rectangle {
    id: root

    property string text
    property string hoverText: ""
    property bool enabled: true

    signal clicked

    width: Style.strutSize * 2
    height: buttonText.implicitHeight + (radius * 2)
    radius: Style.roundRadius

    color: enabled ? (mouse.containsMouse ? Style.activeColor : Style.themeColor) : Style.disabledThemeColor

    Text {
        id: buttonText
        anchors.centerIn: parent
        color: "white"
        text: (mouse.containsMouse && hoverText != "") ? root.hoverText : root.text
        font.pixelSize: Style.baseFontPixelSize
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        enabled: root.enabled

        hoverEnabled: true

        onClicked: {
            root.clicked();
        }

    }
}
