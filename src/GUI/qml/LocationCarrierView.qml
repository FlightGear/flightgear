import QtQuick 2.4
import FlightGear 1.0
import FlightGear.Launcher 1.0

Item {
    property alias geod: diagram.geod

    CarrierDiagram {
        id: diagram
        anchors.fill: parent

        offsetEnabled:  _location.useCarrierFLOLS
        offsetDistance: _location.offsetDistance
        abeam:          _location.abeam
    }

    Component.onCompleted: {
        syncUIFromController();
    }

    function syncUIFromController()
    {
        if (_location.abeam) {
            abeamRadio.select()
        } else if (_location.useCarrierFLOLS) {
            flolsRadio.select()
        } else {
            parkingRadio.select();
            parkingChoice.syncCurrentIndex();
        }
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

        // set opacity here only, so we don't make the whole summary panel translucent
        Rectangle {
            id: background
            anchors.fill: parent
            z: -1
            opacity: Style.panelOpacity
            color: Style.panelBackground
        }

        RadioButtonGroup {
            id: radioGroup
        }

        Column {
            id: selectionGrid
            spacing: Style.margin
            width: parent.width * 0.3

            StyledText { // heading text
                id: headingText
                anchors {
                    left: parent.left
                    right: parent.right
                    margins: Style.margin
                }

                text: qsTr("Carrier: %1").arg(_location.carrier);
                font.pixelSize: Style.headingFontPixelSize
            }

            // on final approach
            Row {
                anchors.left: parent.left
                anchors.leftMargin: Style.margin
                anchors.right: parent.right
                anchors.rightMargin: Style.margin
                spacing: Style.margin

                RadioButton {
                    id: flolsRadio
                    anchors.verticalCenter: parent.verticalCenter
                    group: radioGroup
                    onClicked: {
                        if (selected) {
                          _location.useCarrierFLOLS = selected;
                          _location.abeam = false;
                        }
                    }
                    selected: _location.useCarrierFLOLS && (!_location.abeam)
                }

                StyledText {
                    text: qsTr("On final approach")
                    anchors.verticalCenter: parent.verticalCenter
                    enabled: flolsRadio.selected
                }
            }

            // Abeam the FLOLS
            Row {
                anchors.left: parent.left
                anchors.leftMargin: Style.margin
                anchors.right: parent.right
                anchors.rightMargin: Style.margin
                spacing: Style.margin

                RadioButton {
                    id: abeamRadio
                    anchors.verticalCenter: parent.verticalCenter
                    group: radioGroup
                    onClicked: {
                        if (selected) _location.abeam = _location.useCarrierFLOLS = true
                    }
                    selected: _location.abeam
                }

                StyledText {
                    text: qsTr("Abeam carrier at 180 degrees")
                    anchors.verticalCenter: parent.verticalCenter
                    enabled: abeamRadio.selected
                }
            }


            // parking row
            Row {
                anchors.left: parent.left
                anchors.leftMargin: Style.margin
                anchors.right: parent.right
                anchors.rightMargin: Style.margin
                spacing: Style.margin

                // hide if there's no parking locations defined for this carrier
                visible: _location.carrierParkings.length > 0

                RadioButton {
                    id: parkingRadio
                    anchors.verticalCenter: parent.verticalCenter
                    group: radioGroup
                    onClicked: {
                        if (selected) {
                          parkingChoice.setLocation();
                          _location.useCarrierFLOLS = false;
                          _location.abeam = false;
                        }
                    }
                    selected : (! _location.abeam) && (! _location.useCarrierFLOLS)
                }

                StyledText {
                    text: qsTr("On deck")
                    anchors.verticalCenter: parent.verticalCenter
                    enabled: parkingRadio.selected
                }

                PopupChoice {
                    id: parkingChoice
                    model: _location.carrierParkings
                 //   displayRole: "modelData"
                    width: parent.width * 0.5
                    anchors.verticalCenter: parent.verticalCenter
                    enabled: parkingRadio.selected

                    onCurrentIndexChanged: {
                        setLocation();
                    }

                    function syncCurrentIndex()
                    {
                        for (var i=0; i < _location.carrierParkings.length; ++i) {
                            if (_location.carrierParkings[i] === _location.carrierParking) {
                                currentIndex = i;
                                return;
                            }
                        }

                        // not found, default to available
                        currentIndex = 0;
                    }

                    function setLocation()
                    {
                        _location.carrierParking = _location.carrierParkings[currentIndex]
                        //diagram.selection = _location.airportParkings[currentIndex]
                    }
                }
            }
        }

        Column {
            id: offsetGrid
            spacing: Style.margin
            anchors.leftMargin: Style.margin
            anchors.right: parent.right
            anchors.rightMargin: Style.margin
            width: parent.width * 0.7
            anchors.verticalCenter: parent.verticalCenter

            // Offset selection
            readonly property bool offsetEnabled: (flolsRadio.selected || abeamRadio.selected)

            Row {
                anchors.right: parent.right
                anchors.left: parent.left
                anchors.leftMargin: Style.margin
                anchors.rightMargin: Style.margin
                width : parent.width * 0.7

                NumericalEdit {
                    id: offsetNmEdit
                    quantity: _location.offsetDistance
                    onCommit: _location.offsetDistance = newValue;
                    label: qsTr("at")
                    unitsMode: Units.Distance
                    live: true
                    anchors.verticalCenter: parent.verticalCenter
                    enabled: offsetGrid.offsetEnabled
                }

                StyledText {
                    text: qsTr(" from the FLOLS (aka the ball)")
                    anchors.verticalCenter: parent.verticalCenter
                    enabled: offsetGrid.offsetEnabled
                }
            }

            Row {
                anchors.right: parent.right
                anchors.left: parent.left
                anchors.leftMargin: Style.margin
                anchors.rightMargin: Style.margin
                width : parent.width * 0.7

                ToggleSwitch {
                    id: airspeedToggle
                    enabled: offsetGrid.offsetEnabled
                    checked: _location.speedEnabled
                    onCheckedChanged: _location.speedEnabled = checked;
                    anchors.verticalCenter: parent.verticalCenter
                }

                NumericalEdit {
                    id: airspeedSpinbox
                    label: qsTr("Airspeed:")
                    unitsMode: Units.SpeedWithoutMach
                    enabled: _location.speedEnabled && offsetGrid.offsetEnabled
                    quantity: _location.airspeed
                    onCommit: _location.airspeed = newValue
                    anchors.verticalCenter: parent.verticalCenter
                }

                Item {
                    height: 1; width: Style.strutSize
                }

                LocationAltitudeRow
                {
                    enabled: offsetGrid.offsetEnabled
                    width: parent.width
                }

            } // of Offset selection

            ToggleSwitch {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.leftMargin: Style.margin
                label: qsTr("Tune navigation radio (TACAN) to carrier")
                checked: _location.tuneNAV1
                onCheckedChanged: {
                    _location.tuneNAV1 = checked
                }
            }
        } // main layout column
    } // main panel rectangle
}
