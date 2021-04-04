import QtQuick 2.4
import FlightGear 1.0

Rectangle {
    id: root
    radius: Style.roundRadius
    color: Style.themeColor
    property url link

    property alias label: label.text

    implicitHeight: label.height + Style.margin
    implicitWidth: label.width + Style.margin * 2

    StyledText {
        id: label
        x: Style.margin
        anchors.verticalCenter: parent.verticalCenter
        color: Style.themeContrastTextColor
        font.underline: mouse.containsMouse
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: root.clickable ? Qt.PointingHandCursor : Qt.ArrowCursor

        onClicked: Qt.openUrlExternally(root.link)
    }
}
