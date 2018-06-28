import QtQuick 2.4
import FlightGear 1.0
import FlightGear.Launcher 1.0
import "."

Item {
    property alias location: navaidData.guid
    property alias geod: diagram.geod

    Positioned {
        id: navaidData
        onGuidChanged: {
            if (guid > 0) {
                diagram.navaid = guid
                _location.setBaseLocation(this)
            }
        }
    }

    NavaidDiagram {
        id: diagram
        anchors.fill: parent

        offsetEnabled: _location.offsetEnabled
        offsetBearingDeg: _location.offsetRadial
        offsetDistanceNm: _location.offsetNm
        headingDeg: _location.headingDeg
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

            StyledText { // heading text
                id: headingText
                width: parent.width
                text: qsTr("Position: %1").arg(geod.toString(0));
                font.pixelSize: Style.headingFontPixelSize
                Binding {
                    when: navaidData.valid
                    target: headingText
                    property: "text"
                    value: "Navaid: " + navaidData.ident + " / " + navaidData.name
                }
            }

            Row {
                height: childrenRect.height
                width: parent.width
                spacing: Style.margin

                ToggleSwitch {
                    id: airspeedToggle
                    checked: _location.speedEnabled
                    onCheckedChanged: _location.speedEnabled = checked;
                }

                IntegerSpinbox {
                    label: qsTr("Airspeed:")
                    suffix: "kts"
                    min: 0
                    max: 10000 // more for spaceships?
                    step: 5
                    maxDigits: 5
                    enabled: _location.speedEnabled
                    value: _location.airspeedKnots
                    onCommit: _location.airspeedKnots = newValue
                }

                Item {
                    // padding
                    width: Style.strutSize
                    height: 1
                }

                ToggleSwitch {
                    id: headingToggle
                    checked: _location.headingEnabled
                    function toggle(newChecked) {
                        _location.headingEnabled  = newChecked;
                    }
                }

                IntegerSpinbox {
                    label: qsTr("Heading:")
                    suffix: "deg" // FIXME use Unicode degree symbol
                    min: 0
                    max: 359
                    live: true
                    maxDigits: 3
                    enabled: _location.headingEnabled
                    value: _location.headingDeg
                    onCommit: _location.headingDeg = newValue
                }
            }

            Row {
                height: childrenRect.height
                width: parent.width
                spacing: Style.margin

                ToggleSwitch {
                    id: altitudeToggle
                    checked: _location.altitudeType !== LocationController.Off

                    function toggle(newChecked) {
                        _location.altitudeType = (newChecked ? LocationController.MSL_Feet
                                                               : LocationController.Off)
                    }
                }

                IntegerSpinbox {
                    label: qsTr("Altitude:")
                    suffix: "ft"
                    min: -1000 // Dead Sea, Schiphol
                    max: 200000
                    step: 100
                    maxDigits: 6
                    enabled: altitudeToggle.checked
                    visible: !altitudeTypeChoice.isFlightLevel
                    value: _location.altitudeFt
                    onCommit: _location.altitudeFt = newValue
                }

                IntegerSpinbox {
                    label: qsTr("Altitude:")
                    prefix: "FL"
                    min: 0
                    max: 1000
                    step: 10
                    maxDigits: 3
                    enabled: altitudeToggle.checked
                    visible: altitudeTypeChoice.isFlightLevel
                    value: _location.flightLevel
                    onCommit: _location.flightLevel = newValue
                }

                PopupChoice {
                    id: altitudeTypeChoice
                    enabled: _location.altitudeType !== LocationController.Off
                    currentIndex: Math.max(0, _location.altitudeType - 1)
                    readonly property bool isFlightLevel: (currentIndex == 2)

                    model: [qsTr("Above mean sea-level (MSL)"),
                            qsTr("Above ground (AGL)"),
                            qsTr("Flight-level")]

                    function select(index)
                    {
                        _location.altitudeType = index + 1;
                    }
                }
            }

            // offset row
            Row {
                ToggleSwitch {
                    id: offsetToggle
                    label: qsTr("Offset ")
                    anchors.verticalCenter: parent.verticalCenter
                    checked: _location.offsetEnabled
                    onCheckedChanged: {
                        _location.offsetEnabled = checked
                    }
                }

                DoubleSpinbox {
                    id: offsetNmEdit
                    value: _location.offsetNm
                    onCommit: _location.offsetNm = newValue
                    min: 0.0
                    max: 40.0
                    suffix: "Nm"
                    maxDigits: 5
                    decimals: 1
                    live: true
                    anchors.verticalCenter: parent.verticalCenter
                    enabled: offsetToggle.checked
                }

                StyledText {
                    text: qsTr(" on bearing ")
                    enabled: _location.offsetEnabled
                    anchors.verticalCenter: parent.verticalCenter
                }

                IntegerSpinbox {
                    id: offsetBearingEdit
                    suffix: "deg" // FIXME use Unicode degree symbol
                    min: 0
                    max: 359
                    maxDigits: 3
                    live: true
                    anchors.verticalCenter: parent.verticalCenter
                    enabled: offsetToggle.checked
                    value: _location.offsetRadial
                    onCommit: _location.offsetRadial = newValue
                }
            }
        } // main layout column
    } // main panel rectangle
}
