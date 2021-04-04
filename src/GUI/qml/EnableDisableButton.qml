import QtQuick 2.0
import FlightGear 1.0

Item {
    id: root

    width: height
    height: icon.implicitHeight + 1

    property bool enable: true

    signal clicked();

    Image {
        id:  icon
        source: "image://colored-icon/hide"
    }

    MouseArea {
        id: mouse
     //   hoverEnabled: true
        onClicked: root.clicked();
        anchors.fill: parent
    }

//    Text {
//        anchors.right: root.left
//        anchors.rightMargin: Style.margin
//        anchors.verticalCenter: root.verticalCenter
//        visible: mouse.containsMouse
//        color: Style.baseTextColor
//        font.pixelSize: Style.baseFontPixelSize
//        text: root.enable ? qsTr("Click_to_disable")
//                            : qsTr("Click_to_enable")
//    }
}
