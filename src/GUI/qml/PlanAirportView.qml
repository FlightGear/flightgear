import QtQuick 2.4
import FlightGear 1.0
import FlightGear.Launcher 1.0

Item {
    id: root
    property alias location: airportData.guid

    Positioned {
        id: airportData
    }


    AirportDiagram {
        id: diagram
        anchors.fill: parent
        airport: airportData.guid

        onClicked: {
            if (pos === null)
                return;
        }

        approachExtensionEnabled: false
    }

}
