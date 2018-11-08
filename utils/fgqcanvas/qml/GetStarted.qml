import QtQuick 2.0

Rectangle {
    id: root
    border.width: 1
    border.color: "orange"
    color: "#1f1f1f"
    height: contentBox.childrenRect.height + 40
    width: 600

    Component.onCompleted: {
        if (!_application.showGettingStarted) {
            root.visible = false;
        }
    }

    Column {
        id: contentBox
        width: parent.width - 20
        y: 20
        spacing: 20

        anchors {
            horizontalCenter: parent.horizontalCenter
        }

        Text {
            width: parent.width
            text: _application.gettingStartedText
            wrapMode: Text.WordWrap
            color: "white"
        }

        Row {
            id: buttonRow
            spacing: 20
            anchors.horizontalCenter: parent.horizontalCenter
            height: childrenRect.height

            Button {
                label: qsTr("Okay")
                onClicked: root.visible = false
                width: 150
            }

            Button {
                label: qsTr("Don't show again")
                onClicked: {
                    _application.showGettingStarted = false;
                    root.visible = false
                }
                width: 150
            }
        }
    }
}
