import QtQuick 2.4
import FlightGear 1.0

SettingControl {
    id: root
    property alias choices: popup.model
    property alias displayRole: popup.displayRole

    property alias selectedIndex: popup.currentIndex
    property int defaultIndex: 0

    // alias for save+restore
    property alias value: popup.currentIndex
    property alias defaultValue: root.defaultIndex

    implicitHeight: popup.height + Style.margin + description.height

    PopupChoice {
        id: popup
        label: root.label
        enabled: root.enabled
        width: root.width
    }

    SettingDescription {
        id: description
        enabled: root.enabled
        text: root.description
        anchors.top: popup.bottom
        anchors.topMargin: Style.margin
        width: root.width
    }

     function setValue(newValue)
     {
         popup.currentIndex =  newValue;
     }
}
