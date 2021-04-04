import QtQuick 2.4
import FlightGear 1.0
import FlightGear.Launcher 1.0

Rectangle {
    id: root

    property alias currentIndex: aircraft.variant
    property alias aircraftUri: aircraft.uri

    AircraftInfo {
        id: aircraft
    }

    color: "white"
    border.width: 1
    border.color: Style.minorFrameColor
    width: itemsColumn.width
    height: itemsColumn.height
    clip: true

    property int popupFontPixelSize: 0

    // Overlay control properties
    property bool dismissOnClickOutside: true
    
    signal select(var index);

    y: -OverlayShared.globalOverlay.verticalOverflowFor(height)
    x: -width / 2 // align horizontal center

    Component.onCompleted: {
        computeMenuWidth();
    }

    function close()
    {
        OverlayShared.globalOverlay.dismissOverlay()
    }

    function doSelect(index)
    {
        select(index);
        close();
    }

    function computeMenuWidth()
    {
        var minWidth = 0;
        for (var i = 0; i < choicesRepeater.count; i++) {
            minWidth = Math.max(minWidth, choicesRepeater.itemAt(i).implicitWidth);
        }

        itemsColumn.width = minWidth
    }

    Column {
        id: itemsColumn
        spacing: Style.margin

        Repeater {
            id: choicesRepeater
            model: aircraft.variantNames
            delegate: Item {
                width: itemsColumn.width
                height: choiceText.implicitHeight
                implicitWidth: choiceText.implicitWidth + (Style.margin * 2)

                Text {
                    id: choiceText
                    text: modelData
                    x: Style.margin
                    width: parent.width - (Style.margin * 2)

                    // allow override the size in case the title size is enormous
                    font.pixelSize: (root.popupFontPixelSize > 0) ? root.popupFontPixelSize 
                                            : Style.baseFontPixelSize * 2

                    color: choiceArea.containsMouse ? Style.themeColor : Style.baseTextColor
                }

                MouseArea {
                    id: choiceArea
                    hoverEnabled: true
                    anchors.fill: parent
                    onClicked: {
                        root.doSelect(model.index)
                    }
                }
            } // of delegate Item
        } // menu item repeater
    } // of menu contents column

}
