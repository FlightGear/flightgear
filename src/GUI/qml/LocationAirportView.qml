import QtQuick 2.4
import FlightGear 1.0
import FlightGear.Launcher 1.0

Item {
    id: root
    property alias location: airportData.guid

    Positioned {
        id: airportData
        onGuidChanged: _location.setBaseLocation(this)
    }

    GettingStartedScope.controller: tips.controller

    readonly property bool isHeliport: airportData.type === Positioned.Heliport
    readonly property bool haveParking: airportData.airportHasParkings

    AirportDiagram {
        id: diagram
        anchors.fill: parent
        airport: airportData.guid

        onClicked: {
            if (pos === null)
                return;

            // only allow selction of helipads if the aircraft type in
            // use is a helicopter.
            if ((pos.type == Positioned.Helipad) && (_launcher.aircraftType != LauncherController.Helicopter)) {
                return;
            }

            _location.setDetailLocation(pos)
            diagram.selection = pos
            syncUIFromController();
        }

        approachExtensionEnabled: _location.onFinal
        approachExtension: _location.offsetDistance

        GettingStartedTip {
            tipId: "locationDiagram"
            anchors.centerIn: parent
            arrow: GettingStartedTip.NoArrow
            text: qsTr("Click here to select a runway or parking position, and drag to pan. Mouse-wheel zooms in and out.")
        }
    }

    // not very declarative, try to remove this over time
    function syncUIFromController()
    {
        if (_location.useAvailableParking || (_location.detail.type === Positioned.Parking)) {
            parkingRadio.select()
            parkingChoice.syncCurrentIndex();
        } else if (_location.detail.type === Positioned.Helipad) {
            helipadRadio.select();
            helipadChoice.syncCurrentIndex();
        } else {
            runwayRadio.select();
            runwayChoice.syncCurrentIndex();
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
        clip: true

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
            color: Style.panelBackground
        }

        Column {
            id: selectionGrid
            spacing: Style.margin
            width: parent.width

            StyledText { // heading text
                id: airportHeading
                anchors {
                    left: parent.left
                    right: parent.right
                    margins: Style.margin
                }

                font.pixelSize: Style.headingFontPixelSize
                text: isHeliport ? qsTr("Heliport: ") + airportData.ident + " / " + airportData.name
                                 : qsTr("Airport: ") + airportData.ident + " / " + airportData.name
            }

            Row {
                width: parent.width
                spacing: Style.margin
                visible: !root.isHeliport

                RadioButton {
                    id: runwayRadio
                    anchors.verticalCenter: parent.verticalCenter
                    group: radioGroup

                    onClicked: {
                        if (selected) runwayChoice.setLocation();
                    }
                }

                StyledText {
                    text: qsTr("Runway")
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

            // runway offset
            ToggleBox {
                id: onFinalBox

                visible: !root.isHeliport
                anchors.left: parent.left
                anchors.leftMargin: Style.strutSize
                height: onFinalContents.height + onFinalContents.y + Style.margin
                anchors.right: parent.right
                anchors.rightMargin: Style.margin

                enabled: runwayRadio.selected
                selected: _location.onFinal

                label: qsTr("On final approach")

                onSelectedChanged: _location.onFinal = selected
                readonly property bool enableOnFinal: enabled && selected

                Column {
                    id: onFinalContents
                    y: parent.contentVerticalOffset
                    spacing: Style.margin
                    width: parent.width

                    Row {
                        height: offsetNmEdit.height
                        NumericalEdit {
                            id: offsetNmEdit
                            quantity: _location.offsetDistance
                            onCommit: _location.offsetDistance = newValue;
                            label: qsTr("At")
                            unitsMode: Units.Distance
                            live: true
                            anchors.verticalCenter: parent.verticalCenter
                            enabled: onFinalBox.enableOnFinal
                        }

                        StyledText {
                            text: qsTr(" from the threshold")
                            anchors.verticalCenter: parent.verticalCenter
                            enabled: onFinalBox.enableOnFinal
                        }

                        Item {
                            height: 1; width: Style.strutSize
                        }

                        ToggleSwitch {
                            id: airspeedToggle
                            enabled: onFinalBox.enableOnFinal
                            checked: _location.speedEnabled
                            onCheckedChanged: _location.speedEnabled = checked;
                            anchors.verticalCenter: parent.verticalCenter
                        }

                        NumericalEdit {
                            id: airspeedSpinbox
                            label: qsTr("Airspeed:")
                            unitsMode: Units.SpeedWithoutMach
                            enabled: _location.speedEnabled && onFinalBox.enableOnFinal
                            quantity: _location.airspeed
                            onCommit: _location.airspeed = newValue
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }

                    LocationAltitudeRow
                    {
                        enabled: onFinalBox.enableOnFinal
                        width: parent.width
                    }
                } // of column
            } // of runway offset group


            ToggleSwitch {
                x: Style.strutSize
                enabled:runwayRadio.selected
                visible: !root.isHeliport
                // enable if selected runway has ILS
                label: qsTr("Tune navigation radio (NAV1) to runway localizer")
                checked: _location.tuneNAV1

                onCheckedChanged: {
                    _location.tuneNAV1 = checked
                }
            }

            // helipads row
            Row {
                width: parent.width
                spacing: Style.margin
                visible: (_launcher.aircraftType === LauncherController.Helicopter) && ( _location.airportHelipads.length > 0)

                RadioButton {
                    id: helipadRadio
                    anchors.verticalCenter: parent.verticalCenter
                    group: radioGroup

                    onClicked: {
                        if (selected) helipadChoice.setLocation();
                    }
                }

                StyledText {
                    text: qsTr("Pad")
                    anchors.verticalCenter: parent.verticalCenter
                }

                PopupChoice {
                    id: helipadChoice
                    model: _location.airportHelipads
                    displayRole: "ident"
                    width: parent.width * 0.5
                    anchors.verticalCenter: parent.verticalCenter
                    enabled: helipadRadio.selected

                    onCurrentIndexChanged: {
                        setLocation();
                    }

                    function setLocation()
                    {
                        _location.setDetailLocation(_location.airportHelipads[currentIndex])
                        diagram.selection = _location.airportHelipads[currentIndex] 
                    }

                    function syncCurrentIndex()
                    {
                        for (var i=0; i < _location.airportHelipads.length; ++i) {
                            if (_location.airportHelipads[i].equals(_location.detail)) {
                                currentIndex = i;
                                return;
                            }
                        }

                        currentIndex = 0;
                    }
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

                StyledText {
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
                        console.info("Couldn't find parking at airport:" + _location.detail.ident)
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

    GettingStartedTipLayer {
        id: tips
        anchors.fill: parent
        scopeId: "locationAirportDetails"
    }
}
