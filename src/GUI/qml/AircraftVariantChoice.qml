import QtQuick 2.4
import QtQuick.Window 2.0
import "." // -> forces the qmldir to be loaded

import FlightGear.Launcher 1.0


Rectangle {
    id: root

    implicitHeight: title.implicitHeight

    radius: 4
    border.color: Style.frameColor
    border.width: headingMouseArea.containsMouse ? 1 : 0
    color: headingMouseArea.containsMouse ? "#7fffffff" : "transparent"

    readonly property bool __enabled: aircraftInfo.numVariants > 1

    property alias aircraft: aircraftInfo.uri
    property alias currentIndex: aircraftInfo.variant
    property alias fontPixelSize: title.font.pixelSize
    property int popupFontPixelSize: 0

    // expose this to avoid a second AircraftInfo in the compact delegate
    // really should refactor to put the AircraftInfo up there.
    readonly property string pathOnDisk: aircraftInfo.pathOnDisk

    signal selected(var index)

    Text {
        id: title

        anchors.verticalCenter: parent.verticalCenter
        anchors.right: upDownIcon.left
        anchors.left: parent.left
        anchors.leftMargin: 4
        anchors.rightMargin: 4

        horizontalAlignment: Text.AlignHCenter
        font.pixelSize: Style.baseFontPixelSize * 2
        text: aircraftInfo.name
        fontSizeMode: Text.Fit

        elide: Text.ElideRight
        maximumLineCount: 1
        color: headingMouseArea.containsMouse ? Style.themeColor : Style.baseTextColor
    }


    Image {
        id: upDownIcon
        source: "qrc:///up-down-arrow"
        anchors.right: parent.right
        anchors.rightMargin: 8
        anchors.verticalCenter: parent.verticalCenter
        visible: __enabled
    }

    MouseArea {
        id: headingMouseArea
        enabled: __enabled
        anchors.fill: parent
        hoverEnabled: true
        onClicked: {
            var screenPos = _launcher.mapToGlobal(title, Qt.point(0, -title.height * currentIndex))
            if (screenPos.y < 0) {
                // if the popup would appear off the screen, use the first item
                // position instead
                screenPos = _launcher.mapToGlobal(title, Qt.point(0, 0))
            }

            var pop = popup.createObject(root, {x:screenPos.x, y:screenPos.y })
            tracker.window = pop;
            pop.show();
        }
    }

    AircraftInfo
    {
        id: aircraftInfo
    }

    PopupWindowTracker {
        id: tracker
    }

    Component {
        id: popup

        Window {
            id: popupFrame

            width: root.width
            flags: Qt.Popup
            height: choicesColumn.childrenRect.height
            color: "white"

            Rectangle {
                border.width: 1
                border.color: Style.minorFrameColor
                anchors.fill: parent
            }

            Column {
                id: choicesColumn

                Repeater {
                    // would prefer the model to be conditional on visiblity,
                    // but this trips up the Window sizing on Linux (Ubuntu at
                    // least) and we get a mis-aligned origin
                    model: aircraftInfo.variantNames

                    delegate: Item {
                        width: popupFrame.width
                        height: choiceText.implicitHeight

                        Text {
                            id: choiceText
                            text: modelData

                            // allow override the size in case the title size is enormous
                            font.pixelSize: (root.popupFontPixelSize > 0) ? root.popupFontPixelSize : title.font.pixelSize

                            color: choiceArea.containsMouse ? Style.themeColor : Style.baseTextColor
                            anchors {
                                left: parent.left
                                right: parent.right
                                margins: 4
                            }
                        }

                        MouseArea {
                            id: choiceArea
                            hoverEnabled: true
                            anchors.fill: parent
                            onClicked: {
                                popupFrame.close()
                                root.selected(model.index)
                            }
                        }
                    } // of delegate Item
                } // of Repeater
            }
        } // of popup frame
    } // of popup component
}
