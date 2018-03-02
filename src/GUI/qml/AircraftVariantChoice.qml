import QtQuick 2.0
import QtQuick.Window 2.0
import "." // -> forces the qmldir to be loaded

import FlightGear.Launcher 1.0


Rectangle {
    id: root

    implicitHeight: title.implicitHeight

    radius: 4
    border.color: "#68A6E1"
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
        color: "black"
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

            popupFrame.x = screenPos.x;
            popupFrame.y = screenPos.y;
            popupFrame.visible = true
        }
    }

    AircraftInfo
    {
        id: aircraftInfo
    }

    Window {
        id: popupFrame

        modality: Qt.WindowModal
        width: root.width
        flags: Qt.Popup
        height: choicesColumn.childrenRect.height
        visible: false
        color: "white"

        Rectangle {
            border.width: 1
            border.color: "#afafaf"
            anchors.fill: parent
        }

        Column {
            id: choicesColumn

            Repeater {
                model: popupFrame.visible ? aircraftInfo.variantNames : 0
                delegate: Item {
                    width: popupFrame.width
                    height: choiceText.implicitHeight

                    Text {
                        id: choiceText
                        text: modelData

                        // allow override the size in case the title size is enormous
                        font.pixelSize: (popupFontPixelSize > 0) ? popupFontPixelSize : title.font.pixelSize

                        color: choiceArea.containsMouse ? "#68A6E1" : "black"
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
                            root.selected(model.index)
                            popupFrame.visible = false
                        }
                    }
                } // of delegate Item
            } // of Repeater
        }
    } // of popup frame
}
