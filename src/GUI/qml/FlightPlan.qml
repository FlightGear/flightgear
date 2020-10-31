import QtQuick 2.4
import QtQuick.Controls 2.2
import FlightGear.Launcher 1.0
import FlightGear 1.0
import "."

Item {
    id: root

    // same name as Summary page
    signal showSelectedLocation();

    Flickable {
        id: flick
        height: parent.height
        width: parent.width
        flickableDirection: Flickable.VerticalFlick
        contentHeight: contents.childrenRect.height + Style.margin * 2
        ScrollBar.vertical: ScrollBar {}

        Component.onCompleted: {
            if (_launcher.flightPlan.cruiseSpeed.value === 0.0) {
                _launcher.flightPlan.cruiseSpeed = _launcher.selectedAircraftInfo.cruiseSpeed
            }

            if (_launcher.flightPlan.cruiseAltitude.value === 0.0) {
                _launcher.flightPlan.cruiseAltitude = _launcher.selectedAircraftInfo.cruiseAltitude
            }

            _launcher.flightPlan.aircraftType = _launcher.selectedAircraftInfo.icaoType
            route.text = _launcher.flightPlan.icaoRoute
        }

        Column
        {
            id: contents
            width: parent.width - (Style.margin * 2)
            x: Style.margin
            y: Style.margin
            spacing: Style.margin

            Row {
                width: parent.width
                spacing: Style.margin
                height: loadButton.height

                ToggleSwitch {
                    label: qsTr("Fly with a flight-plan")
                    checked: _launcher.flightPlan.enabled
                    function toggle(newChecked) {
                        _launcher.flightPlan.enabled = newChecked
                    }
                    anchors.verticalCenter: parent.verticalCenter
                }

                Button {
                    id: loadButton
                    text: qsTr("Load");
                    onClicked: {
                        var ok = _launcher.flightPlan.loadPlan();
                        if (ok) {
                            route.text = _launcher.flightPlan.icaoRoute;

                            // we don't use a binding here, manually synchronise
                            // these values
                            departureEntry.selectAirport(_launcher.flightPlan.departure.guid);
                            destinationICAO.selectAirport(_launcher.flightPlan.destination.guid);
                        }
                    }
                }

                Button {
                    text: qsTr("Save");
                    onClicked: _launcher.flightPlan.savePlan();
                }

                Button {
                    text: qsTr("Clear");
                    onClicked: {
                        _launcher.flightPlan.clearPlan();
                        route.text = "";
                    }
                }
            }

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
                    id: aircraftIdent
                    placeholder: "D-FGFS"
                    suggestedWidthString: "XXXXXX";
                    anchors.verticalCenter: parent.verticalCenter
                    text: _launcher.flightPlan.callsign

                    onTextChanged: {
                        _launcher.flightPlan.callsign = text
                    }
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
                    text: _launcher.flightPlan.aircraftType

                    onTextChanged: {
                        _launcher.flightPlan.aircraftType = text
                    }
                }
            }

            Row {
                height: childrenRect.height
                width: parent.width
                spacing: Style.margin

                PopupChoice {
                    id: flightRules
                    label: qsTr("Flight rules:")
                    model: [qsTr("VFR"), qsTr("IFR")] // initially IFR (Y), initially VFR (Z)

                    Component.onCompleted: {
                        select(_launcher.flightPlan.flightRules);
                    }

                    onCurrentIndexChanged: {
                        _launcher.flightPlan.flightRules = currentIndex;
                    }
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

                    Component.onCompleted: {
                        select(_launcher.flightPlan.flightType);
                    }

                    onCurrentIndexChanged: {
                        _launcher.flightPlan.flightType = currentIndex;
                    }
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
                    id: departureEntry
                    label: qsTr("Departure airport:")

                    Component.onCompleted: {
                        selectAirport(_launcher.flightPlan.departure.guid)
                    }

                    onPickAirport: {
                        selectAirport(guid)
                        _launcher.flightPlan.departure = airport
                    }

                    onClickedName: {
                        detailLoader.airportGuid = airport.guid
                        detailLoader.sourceComponent = airportDetails;
                    }

                   KeyNavigation.tab: departureTime
                }

                // padding
                Item { width: Style.strutSize; height: 1 }

                TimeEdit {
                    id: departureTime
                    label: qsTr("Departure time:")
                }
            }

            Row {
                height: childrenRect.height
                width: parent.width
                spacing: Style.margin
                visible: _launcher.flightPlan.departure.valid &&
                         (_launcher.flightPlan.departure.guid !== _location.base.guid)

                ClickableText {
                    width: parent.width
                    text: qsTr("The flight-plan departure airport (%1) is different to the " +
                               "initial location (%2). Click here to set the initial location to " +
                               "the flight-plan's airport.").
                          arg(_launcher.flightPlan.departure.name).arg(_launcher.location.description)

                    onClicked: {
                        _location.setBaseLocation(_launcher.flightPlan.departure)
                        root.showSelectedLocation();
                    }
                }
            }

            // cruising speed + level
            Row {
                height: childrenRect.height
                width: parent.width
                spacing: Style.margin

                NumericalEdit {
                    label: qsTr("Cruise speed:")
                    unitsMode: Units.Speed
                    quantity: _launcher.flightPlan.cruiseSpeed
                    onCommit: {
                        _launcher.flightPlan.cruiseSpeed = newValue
                    }
                    KeyNavigation.tab: cruiseAltitude

                }

                // padding
                Item { width: Style.strutSize; height: 1 }

                NumericalEdit {
                    id: cruiseAltitude
                    label: qsTr("Cruise altitude:")
                    unitsMode: Units.AltitudeIncludingMeters
                    quantity: _launcher.flightPlan.cruiseAltitude
                    onCommit: _launcher.flightPlan.cruiseAltitude = newValue
                }
            }

            StyledText {
                width: parent.width
                text: qsTr("Route")
            }

            PlainTextEditBox {
                id: route
                width: parent.width
                enabled: _launcher.flightPlan.departure.valid && _launcher.flightPlan.destination.valid

                onEditingFinished: {
                    var ok = _launcher.flightPlan.tryParseRoute(text);
                }
            }

            Row {
                height: generateRouteButton.height
                width: parent.width
                spacing: Style.margin

                Button {
                    id: generateRouteButton
                    text: qsTr("Generate route")
                    enabled: route.enabled
                    onClicked: {
                        var ok = _launcher.flightPlan.tryGenerateRoute();
                        if (ok) {
                            route.text = _launcher.flightPlan.icaoRoute;
                        }
                    }
                    anchors.verticalCenter: parent.verticalCenter
                }

                PopupChoice {
                    id: routeNetwork
                    label: qsTr("Using")
                    model: [qsTr("High-level (Jet) airways"),
                            qsTr("Low-level (Victor) airways"),
                            qsTr("High- & low-level airways")]
                    anchors.verticalCenter: parent.verticalCenter
                }

                Button {
                    text: qsTr("View route")
                    onClicked: {
                        detailLoader.airportGuid = 0
                        detailLoader.sourceComponent = routeDetails;
                    }
                    anchors.verticalCenter: parent.verticalCenter
                }

                Button {
                    text: qsTr("Clear route")
                    onClicked: {
                        _launcher.flightPlan.clearRoute();
                        route.text = "";
                    }
                    anchors.verticalCenter: parent.verticalCenter
                }
            }

            RouteLegsView
            {
                id: legsView
                width: parent.width

                onClickedLeg: {
                    detailLoader.airportGuid = 0
                    detailLoader.legIndex = index
                    detailLoader.sourceComponent = routeDetails;
                }
            }

            Row {
                height: childrenRect.height
                width: parent.width
                spacing: Style.margin

                AirportEntry {
                    id: destinationICAO
                    label: qsTr("Destination airport:")

                    Component.onCompleted: {
                        selectAirport(_launcher.flightPlan.destination.guid)
                    }

                    onPickAirport: {
                        selectAirport(guid)
                        _launcher.flightPlan.destination = airport
                    }

                    onClickedName: {
                        detailLoader.airportGuid = airport.guid
                        detailLoader.sourceComponent = airportDetails;
                    }
                }

                Item { width: Style.strutSize; height: 1 }

                TimeEdit {
                    id: enrouteEstimate
                    isDuration: true

                    label: qsTr("Estimated enroute time:")

                    Component.onCompleted: {
                        setDurationMinutes(_launcher.flightPlan.estimatedDurationMinutes)
                    }

                    onValueChanged: {
                        _launcher.flightPlan.estimatedDurationMinutes = value.getHours() * 60 + value.getMinutes();
                    }
                }

                Item { width: Style.strutSize; height: 1 }

                StyledText
                {
                    text: qsTr("Total distance: %1").arg(_launcher.flightPlan.totalDistanceNm);
                }
            }

            Row {
                height: childrenRect.height
                width: parent.width
                spacing: Style.margin

                AirportEntry {
                    id: alternate1
                    label: qsTr("Alternate airport:")

                    Component.onCompleted: {
                        selectAirport(_launcher.flightPlan.alternate.guid)
                    }

                    onPickAirport: {
                        selectAirport(guid)
                        _launcher.flightPlan.alternate = airport
                    }

                    onClickedName: {
                        detailLoader.airportGuid = airport.guid
                        detailLoader.sourceComponent = airportDetails;
                    }
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
                text: _launcher.flightPlan.remarks

                onEditingFinished: {
                    _launcher.flightPlan.remarks = text;
                }
            }
        } // of main column

    } // of flickable

    Component {
        id: airportDetails
        PlanAirportView {
            id: airportView
        }
    }

    Component {
        id: routeDetails
        PlanRouteDetails {
            id: routeView
        }
    }

    Loader {
        id: detailLoader
        anchors.fill: parent
        visible: sourceComponent != null

        property var airportGuid
        property int legIndex

        onStatusChanged: {
            if (status == Loader.Ready) {
                if (item.hasOwnProperty("location")) {
                    item.location = airportGuid
                }

                if (item.hasOwnProperty("legIndex")) {
                    item.legIndex = legIndex
                }
            }
        }
    }

    BackButton {
        id: backButton
        anchors { left: parent.left; top: parent.top; margins: Style.margin }
        visible: detailLoader.visible
        onClicked: {
            detailLoader.sourceComponent = null
        }
    }
}
