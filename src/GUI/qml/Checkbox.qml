import QtQuick 2.4

Item {
    property bool checked: false
    property alias label: label.text

    implicitWidth: checkBox.width + label.width + 16
    implicitHeight: label.height

    Rectangle {
        id: checkBox
        width: 18
        height: 18
        border.color: mouseArea.containsMouse ? "#68A6E1" : "#9f9f9f"
        border.width: 1
        anchors.left: parent.left
        anchors.leftMargin: 8
        anchors.verticalCenter: parent.verticalCenter

        Rectangle {
            width: 12
            height: 12
            anchors.centerIn: parent
            id: checkMark
            color: "#9f9f9f"
            visible: checked
        }
    }

    Text {
        id: label
        anchors.left: checkBox.right
        anchors.leftMargin: 8
        anchors.verticalCenter: parent.verticalCenter
    }

    MouseArea {
        anchors.fill: parent
        id: mouseArea
        hoverEnabled: true
        onClicked: {
            checked = !checked
        }
    }
}
