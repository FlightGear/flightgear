import QtQuick 2.4
import FlightGear 1.0

Text {
    id: root
    signal clicked();

    property bool clickable: true
    property bool enabled: true
    property color baseTextColor: Style.baseTextColor
    color: enabled ? (mouse.containsMouse ? Style.themeColor : baseTextColor) : Style.disabledTextColor
    font.pixelSize: Style.baseFontPixelSize
    font.underline: mouse.containsMouse

    MouseArea {
        id: mouse

        enabled: root.clickable && root.enabled
        hoverEnabled: root.clickable && root.enabled
        anchors.fill: parent

        onClicked: parent.clicked();

        cursorShape: root.clickable ? Qt.PointingHandCursor : Qt.ArrowCursor
    }
}
