import QtQuick 2.2
import FlightGear.Launcher 1.0 as FG

ListHeaderBox {
    id: root
    property int margin: 6

    contents: [
        Text {
            id: updateAllText
            anchors {
                left: parent.left
                right: updateAllButton.left
                margins: root.margin
                verticalCenter: parent.verticalCenter
            }

            text: qsTr("%1 aircraft have updates available - download and install them now?").
                      arg( _launcher.baseAircraftModel.aircraftNeedingUpdated);
            wrapMode: Text.WordWrap
        },

        Button {
            id: updateAllButton
            text: qsTr("Update all")
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: notNowButton.left
            anchors.rightMargin: root.margin

            onClicked: {
                _launcher.requestUpdateAllAircraft();
                _launcher.baseAircraftModel.showUpdateAll = false
            }
        },

        Button {
            id: notNowButton
            text: qsTr("Not now")
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right
            anchors.rightMargin: root.margin

            onClicked: {
                _launcher.baseAircraftModel.showUpdateAll = false
            }
        }
    ]
}
