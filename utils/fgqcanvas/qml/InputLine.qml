import QtQuick 2.2

Item {
    id: root

    property alias text: input.text
    property alias label: labelText.text

    signal editingFinished

    implicitHeight: 30
    implicitWidth: 200

    Text {
        id: labelText

        anchors.left: parent.left
        anchors.right: inputFrame.left
        anchors.verticalCenter: parent.verticalCenter
    }

    Rectangle {
        id: inputFrame
        border.width: 2
        border.color: input.focus ? "orange" : "#9f9f9f"
        color: "#3f3f3f"
        width: parent.width * 0.5
        anchors.right: parent.right
        height: root.height

        TextInput {
            id: input

            anchors {
                left: parent.left
                leftMargin: 8
                right: parent.right
                rightMargin: 8
                verticalCenter: parent.verticalCenter
            }

            onActiveFocusChanged: {
                if (activeFocus) {
                    selectAll();
                }
            }

            verticalAlignment: Text.AlignVCenter

            onEditingFinished: {
                root.editingFinished();
            }

            color: "#9f9f9f"
        }
    }
}
