import QtQuick 2.4
import FlightGear 1.0
import FlightGear.Launcher 1.0
import "."

Item {
    property alias geod: diagram.geod

    NavaidDiagram {
        id: diagram
        anchors.fill: parent

        offsetEnabled: _location.offsetEnabled
        offsetBearing: _location.offsetRadial
        offsetDistance: _location.offsetDistance
        heading: _location.heading
    }

    Component.onCompleted: {
        syncUIFromController();
    }

    function syncUIFromController()
    {
        if (_location.useCarrierFLOLS) {
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

        // set opacity here only, so we don't make the whole summary pannel translucent
        Rectangle {
            id: background
            anchors.fill: parent
            z: -1
            opacity: Style.panelOpacity
            color: "white"
        }

        RadioButtonGroup {
            id: radioGroup
        }

        Column {
            id: selectionGrid
            spacing: Style.margin
            width: parent.width

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

            // on FLOLS offset
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
                        if (selected) _location.useCarrierFLOLS = selected
                    }
                    selected: _location.useCarrierFLOLS
                }

                StyledText {
                    text: qsTr("On final approach")
                    anchors.verticalCenter: parent.verticalCenter
                    enabled: flolsRadio.selected
                }


                NumericalEdit {
                    id: offsetNmEdit
                    quantity: _location.offsetDistance
                    onCommit: _location.offsetDistance = newValue;
                    label: qsTr("at")
                    unitsMode: Units.Distance
                    live: true
                    anchors.verticalCenter: parent.verticalCenter
                    enabled: flolsRadio.selected
                }

                StyledText {
                    text: qsTr(" from the FLOLS (aka the ball)")
                    anchors.verticalCenter: parent.verticalCenter
                    enabled: flolsRadio.selected
                }

                Item {
                    height: 1; width: Style.strutSize
                }

                ToggleSwitch {
                    id: airspeedToggle
                    enabled: flolsRadio.selected
                    checked: _location.speedEnabled
                    onCheckedChanged: _location.speedEnabled = checked;
                    anchors.verticalCenter: parent.verticalCenter
                }

                NumericalEdit {
                    id: airspeedSpinbox
                    label: qsTr("Airspeed:")
                    unitsMode: Units.SpeedWithoutMach
                    enabled: _location.speedEnabled && flolsRadio.selected
                    quantity: _location.airspeed
                    onCommit: _location.airspeed = newValue
                    anchors.verticalCenter: parent.verticalCenter
                }
            } // of FLOLS row

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
                        if (selected) parkingChoice.setLocation();
                    }
                }

                StyledText {
                    text: qsTr("Parking")
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

            ToggleSwitch {
                anchors.left: parent.left
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
