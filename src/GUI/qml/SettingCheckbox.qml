import QtQuick 2.0
import "."

SettingControl {
    id: root

    property alias checked: toggle.checked
    property alias value: toggle.checked
    property bool defaultValue: false

    // should we set the option, if the current value matches the default?
    // this is used to suppress setting needless options to their default
    // value
    property bool setIfDefault: false

    ToggleSwitch {
        id: toggle
        label: root.label
        enabled: root.enabled
    }

    SettingDescription {
        id: description
        enabled: root.enabled
        text: root.description
        anchors.top: toggle.bottom
        anchors.topMargin: Style.margin
        width: root.width
    }

    function apply()
    {
        if (option == "")
            return;

        if (setIfDefault || (value != defaultValue)) {
            _config.setEnableDisableOption(option, checked)
        }
    }
}
