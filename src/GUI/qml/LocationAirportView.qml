import QtQuick 2.4
import FlightGear 1.0
import FlightGear.Launcher 1.0
import "."

Item {
    id: root
    property alias location: airportData.guid

    Positioned {
        id: airportData
        onGuidChanged: _location.setBaseLocation(this)
    }

    readonly property bool isHeliport: airportData.type === Positioned.Heliport
    readonly property bool haveParking: airportData.airportHasParkings

    AirportDiagram {
        id: diagram
        anchors.fill: parent
        airport: airportData.guid

        onClicked: {
            if (pos === null)
                return;

            _location.setDetailLocation(pos)
            diagram.selection = pos
            syncUIFromController();
        }

        approachExtensionNm: _location.onFinal ? _location.offsetNm : -1.0
    }

    // not very declarative, try to remove this over time
    function syncUIFromController()
    {
        runwayRadio.selected = (_location.detail.isRunwayType);
        parkingRadio.selected = (_location.detail.type == Positioned.Parking);

        if (_location.detail.isRunwayType) {
            runwayChoice.syncCurrentIndex();
        } else if (_location.detail.type == Positioned.Parking) {
            parkingChoice.syncCurrentIndex();
        }
    }

    RadioButtonGroup {
        id: radioGroup
    }

    Component.onCompleted: {
        syncUIFromController();
    }

    Rectangle {
        id: panel

        color: "transparent"
        border.width: 1
        border.color: Style.frameColor

        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
            margins: Style.strutSize
        }

        height: selectionGrid.height + Style.margin * 2

        // set opacity here only, so we don't make the whole summary pannel translucent
        Rectangle {
            id: background
            anchors.fill: parent
            z: -1
            opacity: Style.panelOpacity
            color: "white"
        }

        Column {
            id: selectionGrid
            spacing: Style.margin
            width: parent.width

            Text { // heading text
                id: airportHeading
                width: parent.width
                text: isHeliport ? qsTr("Heliport: ") + airportData.ident + " / " + airportData.name
                                 : qsTr("Airport: ") + airportData.ident + " / " + airportData.name
            }

            Row {
                width: parent.width
                spacing: Style.margin

                RadioButton {
                    id: runwayRadio
                    anchors.verticalCenter: parent.verticalCenter
                    group: radioGroup

                    onClicked: {
                        if (selected) runwayChoice.setLocation();
                    }
                }

                Text {
                    text: isHeliport ? qsTr("Pad") : qsTr("Runway")
                    anchors.verticalCenter: parent.verticalCenter
                }

                PopupChoice {
                    id: runwayChoice
                    model: _location.airportRunways
                    displayRole: "ident"
                    width: parent.width * 0.5
                    anchors.verticalCenter: parent.verticalCenter
                    headerText: qsTr("Active")
                    enabled: runwayRadio.selected

                    onCurrentIndexChanged: {
                        setLocation();
                    }

                    function setLocation()
                    {
                        if (currentIndex == -1) {
                            _location.useActiveRunway = true;
                            diagram.selection = null;
                        } else {
                            _location.setDetailLocation(_location.airportRunways[currentIndex])
                            diagram.selection = _location.airportRunways[currentIndex]
                        }
                    }

                    function syncCurrentIndex()
                    {
                        if (_location.useActiveRunway) {
                            currentIndex = -1;
                            return;
                        }

                        for (var i=0; i < _location.airportRunways.length; ++i) {
                            if (_location.airportRunways[i].equals(_location.detail)) {
                                currentIndex = i;
                                return;
                            }
                        }

                        // not found, default to active
                        currentIndex = -1;
                    }
                }
            }

            // runway offset row
            Row {
                x: Style.strutSize

                // no offset for helipads
                visible: !isHeliport

                ToggleSwitch {
                    id: onFinalToggle
                    label: qsTr("On final approach at ")
                    anchors.verticalCenter: parent.verticalCenter
                    checked: _location.onFinal
                    enabled:runwayRadio.selected
                    onCheckedChanged: _location.onFinal = checked
                }

                DoubleSpinbox {
                    id: offsetNmEdit
                    value: _location.offsetNm
                    onCommit: _location.offsetNm = newValue;

                    suffix: "Nm"
                    min: 0.0
                    max: 40.0
                    decimals: 1
                    maxDigits: 5
                    live: true

                    anchors.verticalCenter: parent.verticalCenter
                    enabled: runwayRadio.selected && onFinalToggle.checked
                }

                Text {
                    text: qsTr(" from the threshold")
                    anchors.verticalCenter: parent.verticalCenter
                }
            }

            ToggleSwitch {
                x: Style.strutSize
                // no localizer for helipads
                visible: !isHeliport

                // enable if selected runway has ILS
                label: qsTr("Tune navigation radio (NAV1) to runway localizer")
                checked: _location.tuneNAV1

                onCheckedChanged: {
                    _location.tuneNAV1 = checked
                }
            }

            // parking row
            Row {
                width: parent.width
                spacing: Style.margin

                // hide if there's no parking locations defined for this airport
                visible: haveParking

                RadioButton {
                    id: parkingRadio
                    anchors.verticalCenter: parent.verticalCenter
                    group: radioGroup

                    onClicked: {
                        if (selected) parkingChoice.setLocation();
                    }
                }

                Text {
                    text: qsTr("Parking")
                    anchors.verticalCenter: parent.verticalCenter
                }

                PopupChoice {
                    id: parkingChoice
                    model: _location.airportParkings
                    displayRole: "name"
                    width: parent.width * 0.5
                    anchors.verticalCenter: parent.verticalCenter
                    headerText: qsTr("Available")
                    enabled: parkingRadio.selected

                    onCurrentIndexChanged: {
                        setLocation();
                    }

                    function syncCurrentIndex()
                    {
                        if (_location.useAvailableParking) {
                            currentIndex = -1;
                            return;
                        }

                        for (var i=0; i < _location.airportParkings.length; ++i) {
                            if (_location.airportParkings[i].equals(_location.detail)) {
                                currentIndex = i;
                                return;
                            }
                        }

                        // not found, default to available
                        currentIndex = -1;
                    }

                    function setLocation()
                    {
                        if (currentIndex == -1) {
                            _location.useAvailableParking = true;
                            diagram.selection = null;
                        } else {
                            _location.setDetailLocation(_location.airportParkings[currentIndex])
                            diagram.selection = _location.airportParkings[currentIndex]
                        }
                    }
                }
            }


        } // main layout column
    } // main panel rectangle
}
