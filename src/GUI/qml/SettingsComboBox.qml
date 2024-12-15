import QtQuick 2.4
import FlightGear 1.0

SettingControl {
    id: root

    property alias choices: choicesModel.values
    property var model: null

    property alias displayRole: popup.displayRole

    property alias selectedIndex: popup.currentIndex
    property int defaultIndex: 0

    // alias for save+restore
    property alias value: popup.currentIndex
    property alias defaultValue: root.defaultIndex

    implicitHeight: popup.height + Style.margin + description.height

    StringListModel {
        id: choicesModel
    }

    PopupChoice {
        id: popup
        label: root.label
        enabled: root.enabled
        width: root.width

        // if we have an explicit model, use it, otherwise use
        // our local string list model
        model: root.model ? root.model : choicesModel
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
