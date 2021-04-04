import QtQuick 2.4
import FlightGear 1.0

Text {
    id: choiceText
    property bool isCurrentIndex: false

    signal select(var index);

    height: implicitHeight + Style.margin
    font.pixelSize: Style.baseFontPixelSize
    color: choiceArea.containsMouse ? Style.themeColor : Style.baseTextColor

    MouseArea {
        id: choiceArea
        width: flick.width // full width of the popup
        height: parent.height
        hoverEnabled: true
        onClicked: choiceText.select(model.index)
    }
} // of Text delegate
