import QtQuick 2.4
import FlightGear 1.0

Item {
    property bool enabled: true
    property alias label: switchToggle.label
    property alias selected: switchToggle.checked

    readonly property int contentVerticalOffset: switchToggle.height + Style.margin
    readonly property int __lineTop:  switchToggle.height * 0.5

    ToggleSwitch {
        enabled: parent.enabled
        id: switchToggle
        x: Style.strutSize
    }

    readonly property string lineColor: enabled ? Style.themeColor : Style.disabledThemeColor

    // we want to show a border, but with a cut out. Easiest is to use
    // 1px rectangles as lines
    Rectangle { height: 1; color: lineColor; y: __lineTop; width: switchToggle.x }
    Rectangle { height: 1; color: lineColor; y: __lineTop; anchors { left: switchToggle.right; right: parent.right;} }
    Rectangle { height: 1; color: lineColor; width: parent.width; anchors.bottom: parent.bottom }
}
