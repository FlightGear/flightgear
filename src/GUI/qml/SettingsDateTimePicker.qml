import QtQuick 2.0
import "."

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
        console.info("Custom restore state on SettingsDateTimePicker");
        var rawValue = _config.getValueForKey("", root.setting, defaultValue);
        dateTimeEdit.setDate(rawValue);
    }
}
