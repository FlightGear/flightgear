import QtQuick 2.4
import FlightGear 1.0
import FlightGear.Launcher 1.0

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

    GettingStartedScope.controller: tips.controller

    NavaidDiagram {
        id: diagram
        anchors.fill: parent

        offsetEnabled: _location.offsetEnabled
        offsetBearing: _location.offsetRadial
        offsetDistance: _location.offsetDistance
        heading: _location.heading

        GettingStartedTip {
            tipId: "locationNavDiagram"
            anchors.centerIn: parent
            arrow: GettingStartedTip.NoArrow
            text: qsTr("Drag here to move the map. Mouse-wheel zooms in and out.")
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
            color: Style.panelBackground
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

                text: qsTr("Position: %1").arg(geod.toString(0));
                font.pixelSize: Style.headingFontPixelSize
                Binding {
                    when: navaidData.valid
                    target: headingText
                    property: "text"
                    value: qsTr("Navaid: %1 / %2").arg(navaidData.ident).arg(navaidData.name)
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

                NumericalEdit {
                    label: qsTr("Airspeed:")
                    enabled: _location.speedEnabled
                    quantity: _location.airspeed
                    onCommit: _location.airspeed = newValue
                    unitsMode: Units.Speed
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

                NumericalEdit {
                    label: qsTr("Heading:")
                    // FIXME: support passing a magnetic heading to FG
                    // or compute magvar ourselves
                    unitsMode: Units.HeadingOnlyTrue
                    enabled: _location.headingEnabled
                    quantity: _location.heading
                    onCommit: _location.heading = newValue
                }
            }

            LocationAltitudeRow
            {
                width: parent.width
                unitsMode: Units.AltitudeIncludingMeters
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

                NumericalEdit {
                    id: offsetNmEdit
                    quantity: _location.offsetDistance
                    onCommit: _location.offsetDistance = newValue
                    live: true
                    anchors.verticalCenter: parent.verticalCenter
                    enabled: offsetToggle.checked
                    unitsMode: Units.Distance
                }

                NumericalEdit {
                    label: qsTr(" on bearing ")
                    unitsMode: Units.HeadingOnlyTrue
                    enabled: offsetToggle.checked
                    quantity: _location.offsetRadial
                    onCommit: _location.offsetRadial = newValue
                    live: true
                }
            }
        } // main layout column
    } // main panel rectangle


    GettingStartedTipLayer {
        id: tips
        anchors.fill: parent
        scopeId: "locationNavDetails"
    }
}
