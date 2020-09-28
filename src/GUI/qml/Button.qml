import QtQuick 2.4
import "."

Rectangle {
    id: root

    property string text
    property string hoverText: ""
    property bool enabled: true
    property bool destructiveAction: false

    readonly property string __baseColor: destructiveAction ? Style.destructiveActionColor :
                                                              (invertColor ? "white" : Style.themeColor)

    readonly property string __activeColor: invertColor ? Style.inverseActiveColor : Style.activeColor
    readonly property string __textColor: invertColor ? Style.frameColor : "white"

    signal clicked

    property bool invertColor: false

    width: Math.max(Style.strutSize * 2, buttonText.implicitWidth + radius * 2)
    height: buttonText.implicitHeight + (radius * 2)
    radius: Style.roundRadius

    color: enabled ? (mouse.containsMouse ? __activeColor : __baseColor) : Style.disabledThemeColor

    Text {
        id: buttonText
        anchors.centerIn: parent
        color: root.__textColor
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
