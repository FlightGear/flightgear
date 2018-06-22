import QtQuick 2.4
import "."


Item {
    id: root

    width: Style.strutSize * 2
    height: Style.strutSize * 2

    property alias icon: iconImage.source
    property alias label: label.text

    signal clicked()

    property bool selected: false
    property bool enabled: true

    Rectangle {
        anchors.fill: parent
        visible: root.enabled & (root.selected | mouse.containsMouse)
        color: Style.activeColor
    }

    Image {
        id: iconImage
        anchors.centerIn: parent
        opacity: root.enabled ? 1.0 : 0.5
    }

    Text {
        id: label
        color: "white"
        // enabled appearance is done via opacity to match the icon
        opacity: root.enabled ? 1.0 : 0.5
        width: parent.width
        horizontalAlignment: Text.AlignHCenter
        anchors.top: iconImage.bottom
        anchors.topMargin: Style.margin
    }

    MouseArea {
        id: mouse
        enabled: root.enabled
        anchors.fill: parent
        hoverEnabled: true
        onClicked: root.clicked();
    }
}
