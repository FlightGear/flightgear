import QtQuick 2.2
import FlightGear.Launcher 1.0 as FG
import FlightGear 1.0

ListHeaderBox {
    id: root

    contents: [
        StyledText {
            id: updateAllText
            anchors {
                left: parent.left
                right: updateAllButton.left
                margins: Style.margin
                verticalCenter: parent.verticalCenter
            }

            text: qsTr("%1 aircraft have updates available - download and install them now?").
                      arg(_launcher.aircraftWithUpdatesModel.count)
            wrapMode: Text.WordWrap
        },

        Button {
            id: updateAllButton
            text: qsTr("Update all")
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right
            anchors.rightMargin: Style.margin

            onClicked: {
                _launcher.requestUpdateAllAircraft();
            }
        }
    ]
}
