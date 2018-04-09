import QtQuick 2.4
import QtQuick.Window 2.0
import FlightGear.Launcher 1.0

import "."

Item {
    id: root
    property alias label: label.text

    property var model: undefined
    property string displayRole: "display"
    property bool enabled: true
    property int currentIndex: 0
    property bool __dummy: false

    property alias header: choicesHeader.sourceComponent
    property string headerText: ""

    implicitHeight: Math.max(label.implicitHeight, currentChoiceFrame.height)
    implicitWidth: label.implicitWidth + Style.margin + currentChoiceFrame.__naturalWidth

    Item {
        Repeater {
            id: internalModel
            model: root.model

            Item {
                id: internalModelItem

                // Taken from TableViewItemDelegateLoader.qml to follow QML role conventions
                readonly property var text: model && model.hasOwnProperty(displayRole) ? model[displayRole] // Qml ListModel and QAbstractItemModel
                                                                                       : modelData && modelData.hasOwnProperty(displayRole) ? modelData[displayRole] // QObjectList / QObject
                                                                                                                                            : modelData != undefined ? modelData : "" // Models without role
                readonly property bool selected: root.currentIndex === model.index
                readonly property QtObject modelObj: model
            }
        }
    }

    Component.onCompleted: {
        // hack to force updating of currentText after internalModel
        // has been populated
        __dummy = !__dummy
    }

    onModelChanged: {
         __dummy = !__dummy // force update of currentText
    }

    function haveHeader()
    {
        return headerText !== "";
    }

    function currentText()
    {
        if ((currentIndex == -1) && haveHeader())
            return headerText;

        var foo = __dummy; // fake propery dependency to update this
        var item = internalModel.itemAt(currentIndex);
        if (!item) return "";
        return item.text
    }

    Text {
        id: label
        anchors.left: root.left
        anchors.leftMargin: 8
        anchors.verticalCenter: parent.verticalCenter
        horizontalAlignment: Text.AlignRight
        color: mouseArea.containsMouse ? Style.themeColor :
                                         (root.enabled ? "black" : Style.inactiveThemeColor)
    }

    Rectangle {
        id: currentChoiceFrame
        radius: Style.roundRadius
        border.color: root.enabled ? (mouseArea.containsMouse ? Style.themeColor : Style.minorFrameColor)
                                   : Style.inactiveThemeColor
        border.width: 1
        height: currentChoiceText.implicitHeight + Style.margin
        clip: true

        anchors.left: label.right
        anchors.leftMargin: Style.margin

        // width of current item, or available space after the label
        width: Math.min(root.width - (label.width + Style.margin), __naturalWidth);

        readonly property int __naturalWidth: currentChoiceText.implicitWidth + (Style.margin * 3) + upDownIcon.width
        anchors.verticalCenter: parent.verticalCenter

        Text {
            id: currentChoiceText
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.margins: Style.margin
            text: currentText()
            color: mouseArea.containsMouse ? Style.themeColor : Style.baseTextColor
            elide: Text.ElideRight
            maximumLineCount: 1
        }

        Image {
            id: upDownIcon
            source: "qrc:///up-down-arrow"
            anchors.right: parent.right
            anchors.rightMargin: Style.margin
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    MouseArea {
        anchors.fill: parent
        id: mouseArea
        hoverEnabled: root.enabled
        enabled: root.enabled
        onClicked: {
            var screenPos = _launcher.mapToGlobal(currentChoiceText, Qt.point(0, -currentChoiceText.height * currentIndex))
            if (screenPos.y < 0) {
                // if the popup would appear off the screen, use the first item
                // position instead
                screenPos = _launcher.mapToGlobal(currentChoiceText, Qt.point(0, 0))
            }

            popupFrame.x = screenPos.x;
            popupFrame.y = screenPos.y;
            popupFrame.visible = true
            tracker.window = popupFrame
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

        // choice layout column
        Column {
            id: choicesColumn
            spacing: Style.margin
            x: Style.margin
            y: Style.margin

            // optional header component:
            Loader {
                id: choicesHeader
                active: root.haveHeader()

                // default component is just a plain text element, same as
                // normal items
                sourceComponent: Text {
                    text: root.headerText
                    height: implicitHeight + Style.margin
                    width: popupFrame.width
                }

                height: item ? item.height : 0
                width: item ? item.width : 0

                // essentially the same mouse area as normal items
                MouseArea {
                    width: popupFrame.width // full width of the popup
                    height: parent.height
                    z: -1 // so header can do other mouse behaviours
                    onClicked: {
                        root.currentIndex = -1;
                        popupFrame.visible = false
                    }
                }
            } // of header loader

            // main item repeater
            Repeater {
                id: choicesRepeater
                model: root.model
                delegate:
                    Text {
                        id: choiceText
                        readonly property bool selected: root.currentIndex === model.index

                        // Taken from TableViewItemDelegateLoader.qml to follow QML role conventions
                        text: model && model.hasOwnProperty(displayRole) ? model[displayRole] // Qml ListModel and QAbstractItemModel
                                                                         : modelData && modelData.hasOwnProperty(displayRole) ? modelData[displayRole] // QObjectList / QObject
                                                                                                                              : modelData != undefined ? modelData : "" // Models without role
                        height: implicitHeight + Style.margin

                        MouseArea {
                            width: popupFrame.width // full width of the popup
                            height: parent.height
                            onClicked: {
                                root.currentIndex = model.index
                                popupFrame.visible = false
                            }
                        }
                    } // of Text delegate
            } // text repeater
        } // text column
    } // of popup Window
}
