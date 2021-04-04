import QtQuick 2.4
import FlightGear.Launcher 1.0
import FlightGear 1.0

Row {
    id: root
    height: childrenRect.height
    spacing: Style.margin
    property bool enabled: true
    property alias unitsMode: edit.unitsMode

    ToggleSwitch {
        id: altitudeToggle
        checked: _location.altitudeEnabled

        function toggle(newChecked) {
            _location.altitudeEnabled = newChecked
        }

        enabled: parent.enabled
    }

    readonly property bool __rowEnabled: root.enabled && _location.altitudeEnabled

    NumericalEdit {
        id: edit
        label: qsTr("Altitude:")
        enabled: __rowEnabled
        quantity: _location.altitude
        onCommit: _location.altitude = newValue
        unitsMode: Units.AltitudeIncludingMetersAndAboveField
    }
}
