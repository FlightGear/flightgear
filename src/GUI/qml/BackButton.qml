import QtQuick 2.4
import FlightGear 1.0

Item {
    id: root
    signal clicked
    property bool enabled: true

    width: image.width + Style.margin
    height: image.height + Style.margin

    Image {
        id: image
        anchors.centerIn: parent
        source:  mouse.containsMouse ? "image://colored-icon/back?active" : "image://colored-icon/back"
    }

    MouseArea {
        id: mouse
        hoverEnabled: true
        anchors.fill: parent

        onClicked: {
            root.clicked();
        }
    }
}
