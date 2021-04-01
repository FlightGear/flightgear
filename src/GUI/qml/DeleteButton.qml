import QtQuick 2.4
import "."

Item {
    id: root

    property bool enabled: true
    signal clicked

    width: height
    height: icon.implicitHeight + 1

    Image {
        id: icon
        x: 0
        y: 0
        source: "image://colored-icon/cancel-small?destructive"
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        enabled: root.enabled

        onClicked: {
            root.clicked();
        }

    }
}
