import QtQuick 2.0

Item {
    id: root

    signal menuBack();
    signal closeCanvas();
    signal reconnectCanvas();

    Rectangle {
        anchors.centerIn: parent
        width: buttons.childrenRect.width + 20
        height: buttons.childrenRect.height + 20
        border.width: 1
        border.color: "orange"
        color: "#5f5f5f"

        Column {
            id: buttons
            spacing: 30

            Button {
                label: qsTr("Back")
                onClicked: root.menuBack();
                anchors.horizontalCenter: parent.horizontalCenter
            }

            Button {
                label: qsTr("Close")
                onClicked: root.closeCanvas();
                anchors.horizontalCenter: parent.horizontalCenter
            }

            Button {
                label: qsTr("Reconnect")
                onClicked: root.reconnectCanvas();
                anchors.horizontalCenter: parent.horizontalCenter
            }
        } // of buttons column
    }
}
