import QtQuick 2.4
import FlightGear.Launcher 1.0
import "."

Item {

    Flickable {
        id: flick
        height: parent.height
        width: parent.width - scrollbar.width
        flickableDirection: Flickable.VerticalFlick
        contentHeight: contents.childrenRect.height

        Column
        {
            id: contents
            width: parent.width - (Style.margin * 2)
            x: Style.margin
            spacing: Style.margin

            HeaderBox {
                title: qsTr("Aircraft & flight information")
                width: parent.width
            }

            // date of flight << omit for now

            Row {
                height: aircraftIdent.height
                width: parent.width
                spacing: Style.margin

                StyledText {
                    text: qsTr("Callsign / Flight No.")
                    anchors.verticalCenter: parent.verticalCenter
                }
                LineEdit {
                    // Aircraft identication - callsign (share with MP)

                    id: aircraftIdent
                    placeholder: "D-FGFS"
                    suggestedWidthString: "XXXXXX";
                    anchors.verticalCenter: parent.verticalCenter
                }

                Item { width: Style.strutSize; height: 1 }

                StyledText {
                    text: qsTr("Aircraft type:")
                    anchors.verticalCenter: parent.verticalCenter
                }
                LineEdit {
                    placeholder: "B738"
                    suggestedWidthString: "XXXX";
                    anchors.verticalCenter: parent.verticalCenter
                }
            }

            Row {
                height: childrenRect.height
                width: parent.width
                spacing: Style.margin

                PopupChoice {
                    id: flightRules
                    label: qsTr("Flight rules:")
                    model: ["VFR", "IFR"] // initially IFR (Y), initially VFR (Z)
                }

                Item { width: Style.strutSize; height: 1 }

                PopupChoice {
                    id: flightType
                    label: qsTr("Flight type:")
                    model: [qsTr("Scheduled"),
                            qsTr("Non-scheduled"),
                            qsTr("General aviation"),
                            qsTr("Military"),
                            qsTr("Other")]
                }
            }

            Row {
                height: childrenRect.height
                width: parent.width
                spacing: Style.margin

                PopupChoice {
                    id: wakeTurbulenceCategory
                    label: qsTr("Wake turbulence category:")
                    model: [qsTr("Light"),
                            qsTr("Medium"),
                            qsTr("Heavy"),
                            qsTr("Jumbo")]
                }

                // equipment
                //  - ideally prefill from acft
            }

            HeaderBox {
                title: qsTr("Route")
                width: parent.width
            }

            Row {
                height: childrenRect.height
                width: parent.width
                spacing: Style.margin

                AirportEntry {
                    label: qsTr("Departure airport:")
                }

                // padding
                Item { width: Style.strutSize; height: 1 }

                TimeEdit {
                    id: departureTime
                    label: qsTr("Departure time:")
                }

            }

            // cruising speed + level
            Row {
                height: childrenRect.height
                width: parent.width
                spacing: Style.margin

                IntegerSpinbox {
                    label: qsTr("Cruise speed:")
                    suffix: "kts"
                    min: 0
                    max: 10000 // more for spaceships?
                    step: 5
                    maxDigits: 5
                }

                // padding
                Item { width: Style.strutSize; height: 1 }

                LocationAltitudeRow {

                }
            }

            StyledText {
                width: parent.width
                text: qsTr("Route")
            }

            PlainTextEditBox {
                id: route
                width: parent.width

            }

            Row {
                height: childrenRect.height
                width: parent.width
                spacing: Style.margin

                AirportEntry {
                    id: destinationICAO
                    label: qsTr("Destination airport:")
                }

                Item { width: Style.strutSize; height: 1 }

                TimeEdit {
                    id: enrouteEstimate
                    label: qsTr("Estimate enroute time:")
                }

                Item { width: Style.strutSize; height: 1 }

                AirportEntry {
                    id: alternate1
                    label: qsTr("Alternate airport:")
                }

            }

            HeaderBox {
                title: qsTr("Additional information")
                width: parent.width
            }

            StyledText {
                width: parent.width
                text: qsTr("Remarks")
            }

            PlainTextEditBox {
                id: remarks
                width: parent.width

            }

            // speak to Act-pie guy about passing all this over MP props?
        } // of main column

    } // of flickable

    Scrollbar {
        id: scrollbar
        anchors.right: parent.right
        height: parent.height
        flickable: flick
        visible: flick.contentHeight > flick.height
    }
}
