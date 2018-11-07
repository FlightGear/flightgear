import QtQuick 2.4
import QtQuick.Window 2.0
import "." // -> forces the qmldir to be loaded

import FlightGear.Launcher 1.0


Rectangle {
    id: root

    implicitHeight: title.implicitHeight

    radius: Style.roundRadius
    border.color: Style.frameColor
    border.width: headingMouseArea.containsMouse ? 1 : 0
    color: headingMouseArea.containsMouse ? "#7fffffff" : "transparent"

    readonly property int centerX: width / 2

    readonly property bool __enabled: aircraftInfo.numVariants > 1

    property alias aircraft: aircraftInfo.uri
    property alias currentIndex: aircraftInfo.variant
    property alias fontPixelSize: title.font.pixelSize
    property int popupFontPixelSize: 0

    // expose this to avoid a second AircraftInfo in the compact delegate
    // really should refactor to put the AircraftInfo up there.
    readonly property string pathOnDisk: aircraftInfo.pathOnDisk

    signal selected(var index)




//    Image {
//        id: upDownIcon
//        source: "qrc:///up-down-arrow"
//        x: root.centerX + Math.min(title.implicitWidth * 0.5, title.width * 0.5)
//        anchors.verticalCenter: parent.verticalCenter
//        visible: __enabled
//    }

    Row {
        anchors.centerIn: parent
        height: title.implicitHeight
        width: childrenRect.width

        Text {
            id: title

            anchors.verticalCenter: parent.verticalCenter

            // this ugly logic is to keep the up-down arrow at the right
            // hand side of the text, but allow the font scaling to work if
            // we're short on horizontal space
            width: Math.min(implicitWidth,
                            root.width - (Style.margin * 2) - (__enabled ? upDownIcon.implicitWidth : 0))

            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter

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
           // x: root.centerX + Math.min(title.implicitWidth * 0.5, title.width * 0.5)
            anchors.verticalCenter: parent.verticalCenter
            visible: __enabled
            width: __enabled ? implicitWidth : 0
        }
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

            popupFrame.x = screenPos.x;
            popupFrame.y = screenPos.y;
            popupFrame.show()
            tracker.window = popupFrame
        }
    }

    AircraftInfo
    {
        id: aircraftInfo
    }

    PopupWindowTracker {
        id: tracker
    }

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
                        font.pixelSize: (popupFontPixelSize > 0) ? popupFontPixelSize : title.font.pixelSize

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
                            popupFrame.hide()
                            root.selected(model.index)
                        }
                    }
                } // of delegate Item
            } // of Repeater
        }
    } // of popup frame
}
