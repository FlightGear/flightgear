import QtQuick 2.4
import "."

Item {
    id: root

    property string title: ""
    property int value: 3

    implicitWidth: Style.strutSize * 3
    implicitHeight: label.height

    Text {
        id: label
        anchors.right: ratingRow.left
        anchors.rightMargin: Style.margin
        anchors.left: parent.left

        horizontalAlignment: Text.AlignRight
        text: root.title + ":"
        font.pixelSize: Style.baseFontPixelSize
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
                width: radius * 2
                height: radius * 2
                radius: Style.roundRadius
            }
        }
    }

}
