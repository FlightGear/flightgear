import QtQuick 2.0
import FlightGear 1.0

Item {
    property alias promptText:prompt.text
    property alias yesText: yesButton.text
    property alias noText: noButton.text

    // change UI appearance if yes is a destructive action
    property bool yesIsDestructive: false

    signal accepted()

    signal rejected()

    StyledText {
        id: prompt
        anchors {
            verticalCenter: parent.verticalCenter
            left: parent.left
            right: yesButton.left
            margins: Style.margin
        }

        height: parent.height
        verticalAlignment: Text.AlignVCenter
        wrapMode: Text.WordWrap
    }

    Button {
        id: yesButton
        anchors {
            verticalCenter: parent.verticalCenter
            right: noButton.left
            rightMargin: Style.margin
        }

        destructiveAction: parent.yesIsDestructive
        onClicked: parent.accepted()
    }

    Button {
        id: noButton

        anchors {
            verticalCenter: parent.verticalCenter
            right: parent.right
            rightMargin: Style.margin
        }

        onClicked: parent.rejected();
    }

}
