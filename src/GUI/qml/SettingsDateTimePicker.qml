import QtQuick 2.4
import FlightGear 1.0

SettingControl {
    id: root


    // alias for save+restore
    property alias value: dateTimeEdit.value

    property alias defaultValue: root.now

    readonly property date now: new Date()

    implicitHeight: childrenRect.height

    DateTimeEdit {
        id: dateTimeEdit
        label: root.label
        enabled: root.enabled
        width: root.width
    }

    SettingDescription {
        id: description
        enabled: root.enabled
        text: root.description
        anchors.top: dateTimeEdit.bottom
        anchors.topMargin: Style.margin
        width: root.width
    }

    function restoreState()
    {
        var rawValue = _config.getValueForKey("", root.setting, defaultValue);
        var year = rawValue.getFullYear();
        if (isNaN(year) || (year < 1800)) {
            // assume it's an invalid date, there doesn't seem to be a better
            // way to check date validity from JS :(
            rawValue = defaultValue
        }

        dateTimeEdit.setDate(rawValue);
    }
}
