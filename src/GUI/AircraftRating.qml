import QtQuick 2.0

Item {
    id: root

    property string title: ""
    property int value: 3

    implicitWidth: 160
    implicitHeight: label.height

    Text {
        id: label
        anchors.right: ratingRow.left
        anchors.rightMargin: 6
        anchors.left: parent.left

        horizontalAlignment: Text.AlignRight
        text: root.title + ":"


    }

    Row {
        id: ratingRow

        spacing: 2
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter

        Repeater {
            model: 5

            delegate: Rectangle {
                color: ((model.index + 1) <= root.value) ? "#3f3f3f" : "#cfcfcf"
                width: 12
                height: 12
                radius: 6
            }
        }
    }

}
