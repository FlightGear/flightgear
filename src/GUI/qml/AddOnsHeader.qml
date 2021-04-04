import QtQuick 2.4
import FlightGear 1.0

Item {
    id: root

    property bool showAddButton: false

    property alias title: header.title
    property alias description: description.text
    signal add();

    implicitWidth: parent.width
    implicitHeight: header.height + Style.margin + description.height

    HeaderBox {
        id: header
        width: parent.width

        AddButton {
            id: addButton
            visible: root.showAddButton
            anchors.right: parent.right
            anchors.rightMargin: Style.margin
            anchors.verticalCenter: parent.verticalCenter
            onClicked: root.add()
        }
    }

    StyledText {
        id: description
        width: parent.width
        anchors.top: header.bottom
        anchors.topMargin: Style.margin
        wrapMode: Text.WordWrap
    }
}
