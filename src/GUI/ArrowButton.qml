import QtQuick 2.0

Item {
    id: root
    signal clicked;


    property alias arrow: img.source

    implicitWidth: img.width + 4
    implicitHeight: img.height + 4

    Rectangle {
       visible: mouse.containsMouse
       radius: 3
       anchors.fill: parent
       color: "#1b7ad3"
    }

    Image {
        id: img
        visible: (model.variantCount > 0)
        anchors.centerIn: parent
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true

        onClicked: {
            root.clicked();
        }
    }
}
