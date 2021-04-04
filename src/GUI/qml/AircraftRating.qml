import QtQuick 2.4
import FlightGear 1.0

Item {
    id: root

    property string title: ""
    property int value: 3

    implicitWidth: Style.strutSize * 3
    implicitHeight: label.height

    StyledText {
        id: label
        anchors.right: ratingRow.left
        anchors.rightMargin: Style.margin
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
                color: ((model.index + 1) <= root.value) ? Style.themeColor : Style.disabledTextColor
                width: radius * 2
                height: radius * 2
                radius: Style.roundRadius
            }
        }
    }

}
