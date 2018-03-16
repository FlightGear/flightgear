import QtQuick 2.4

Item {

    width: height
    height: icon.implicitHeight + 1

    signal moveUp();
    signal moveDown();

    Image {
        id: icon
        x: 0
        y: 0
        source: "qrc:///reorder-list-icon"
    }


    MouseArea {
        id: moveUpArea
        height: parent.height / 2
        width: parent.width
        onClicked: {
            parent.moveUp();
        }
    }

    MouseArea {
        id: moveDownArea
        height: parent.height / 2
        width: parent.width
        anchors.bottom: parent.bottom
        onClicked: {
            parent.moveDown();
        }
    }
}
