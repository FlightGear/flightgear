import QtQuick 2.4
import "."
import FlightGear.Launcher 1.0

Row {
    id: root
    height: childrenRect.height
    spacing: Style.margin
    property bool enabled: true

    ToggleSwitch {
        id: altitudeToggle
        checked: _location.altitudeType !== LocationController.Off

        function toggle(newChecked) {
            _location.altitudeType = (newChecked ? LocationController.MSL_Feet
                                                   : LocationController.Off)
        }

        enabled: parent.enabled
    }

    readonly property bool __rowEnabled: root.enabled && altitudeToggle.checked

    IntegerSpinbox {
        label: qsTr("Altitude:")
        suffix: "ft"
        min: -1000 // Dead Sea, Schiphol
        max: 200000
        step: 100
        maxDigits: 6
        enabled: __rowEnabled
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
        enabled: __rowEnabled
        visible: altitudeTypeChoice.isFlightLevel
        value: _location.flightLevel
        onCommit: _location.flightLevel = newValue
    }

    PopupChoice {
        id: altitudeTypeChoice
        enabled: __rowEnabled && (location.altitudeType !== LocationController.Off)
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
