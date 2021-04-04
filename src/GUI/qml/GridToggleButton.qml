import QtQuick 2.0
import FlightGear 1.0

Rectangle {
    id: root
    radius: Style.roundRadius
    border.width: 1
    border.color: Style.themeColor
    width: height
    height: Style.baseFontPixelSize + Style.margin * 2
    color: mouse.containsMouse ? Style.minorFrameColor : Style.backgroundColor

    property bool gridMode: false

    signal clicked();

    Image {
        id:  icon
        width: parent.width - Style.margin
        height: parent.height - Style.margin
        anchors.centerIn: parent

        readonly property string __iconSuffix: mouse.containsMouse ? "?theme" : "?text"
        source: (root.gridMode ? "image://colored-icon/grid-view"
                              : "image://colored-icon/list-view") + __iconSuffix
    }

    MouseArea {
        id: mouse
        hoverEnabled: true
        onClicked: root.clicked();
        anchors.fill: parent
    }

    Text {
        anchors.left: root.right
        anchors.leftMargin: Style.margin
        anchors.verticalCenter: root.verticalCenter
        visible: mouse.containsMouse
        color: Style.baseTextColor
        font.pixelSize: Style.baseFontPixelSize
        text: root.gridMode ? qsTr("Switch to grid view")
                            : qsTr("Switch to list view")
    }
}
