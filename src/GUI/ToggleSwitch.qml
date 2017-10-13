import QtQuick 2.0

Item {
    property bool checked: false
    property alias label: label.text

    implicitWidth: track.width + label.width + 16
    implicitHeight: label.height

    Rectangle {
        id: track
        width: height * 2
        height: 12
        radius: 6

        color: checked ? "#68A6E1" :"#9f9f9f"
        anchors.left: parent.left
        anchors.leftMargin: 8
        anchors.verticalCenter: parent.verticalCenter

        Rectangle {
            id: thumb
            width: 18
            height: 18
            radius: 9

            anchors.verticalCenter: parent.verticalCenter
            color: checked ? "#68A6E1" : "white"
            border.width: 1
            border.color: "#9f9f9f"

            x: checked ? parent.width - (6 + radius) : -3

            Behavior on x {
                NumberAnimation {
                    duration: 250
                }
            }
        }
    }

    Text {
        id: label
        anchors.left: track.right
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
