import QtQuick 2.4
import "."

Item {
    id: root
    signal clicked
    property bool enabled: true

    width: image.width + Style.margin
    height: image.height + Style.margin

    Image {
        id: image
        anchors.centerIn: parent
        source: "qrc:///back-icon"
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
