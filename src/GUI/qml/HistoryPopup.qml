import QtQuick 2.4
import QtQuick.Window 2.0
import FlightGear.Launcher 1.0

import "."

Item {
    id: root

    property var model: undefined
    property string displayRole: "display"
    property bool enabled: true

    implicitHeight: button.height
    implicitWidth: button.width

    signal selected(var index);

    Rectangle {
        id: button

        width: icon.width
        height: icon.height
        radius: Style.roundRadius

        color: enabled ? (mouse.containsMouse ? Style.activeColor : Style.themeColor) : Style.disabledThemeColor

        Image {
            id: icon
            source: "qrc:///history-icon"
            anchors.centerIn: parent
        }

        MouseArea {
            anchors.fill: parent
            id: mouse
            hoverEnabled: root.enabled
            enabled: root.enabled
            onClicked: {
                var screenPos = _launcher.mapToGlobal(button, Qt.point(-popupFrame.width, 0))
                popupFrame.x = screenPos.x;
                popupFrame.y = screenPos.y;
                popupFrame.visible = true
                tracker.window = popupFrame
            }
        }
    }

    PopupWindowTracker {
        id: tracker
    }

    Window {
        id: popupFrame

        flags: Qt.Popup
        height: choicesColumn.childrenRect.height + Style.margin * 2
        width: choicesColumn.childrenRect.width + Style.margin * 2
        visible: false
        color: "white"

        Rectangle {
            border.width: 1
            border.color: Style.minorFrameColor
            anchors.fill: parent
        }

        // text repeater
        Column {
            id: choicesColumn
            spacing: Style.margin
            x: Style.margin
            y: Style.margin

            Repeater {
                id: choicesRepeater
                model: root.model
                delegate:
                    Text {
                        id: choiceText

                        // Taken from TableViewItemDelegateLoader.qml to follow QML role conventions
                        text: model && model.hasOwnProperty(displayRole) ? model[displayRole] // Qml ListModel and QAbstractItemModel
                                                                         : modelData && modelData.hasOwnProperty(displayRole) ? modelData[displayRole] // QObjectList / QObject
                                                                                                                              : modelData != undefined ? modelData : "" // Models without role
                        height: implicitHeight + Style.margin

                        MouseArea {
                            width: popupFrame.width // full width of the popup
                            height: parent.height
                            onClicked: {
                                popupFrame.visible = false
                                root.selected(model.index);
                            }
                        }
                    } // of Text delegate
            } // text repeater
        } // text column
    } // of popup Window
}
