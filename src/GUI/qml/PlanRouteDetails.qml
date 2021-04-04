import QtQuick 2.4
import FlightGear 1.0
import FlightGear.Launcher 1.0

Item {
    id: root

    property alias legIndex: diagram.activeLegIndex

    RouteDiagram {
        id: diagram
        anchors.fill: parent
        flightplan: _launcher.flightPlan
    }

    Button {
        id: previousButton
        text: qsTr("Previous Leg")
        enabled: diagram.activeLegIndex > 0
        onClicked: {
            diagram.activeLegIndex = diagram.activeLegIndex - 1
        }

        anchors.right: root.horizontalCenter
        anchors.bottom: root.bottom
        anchors.margins: Style.margin
    }

    Button {
        text: qsTr("Next Leg")
        width: previousButton.width

        enabled: diagram.activeLegIndex < (diagram.numLegs - 1)
        onClicked: {
            diagram.activeLegIndex = diagram.activeLegIndex + 1
        }

        anchors.left: root.horizontalCenter
        anchors.bottom: root.bottom
        anchors.margins: Style.margin
    }
}
