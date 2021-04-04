import QtQuick 2.4
import FlightGear 1.0

Text {
    property bool enabled: true

    color: enabled ? Style.baseTextColor : Style.disabledTextColor

    font.pixelSize: Style.baseFontPixelSize

    wrapMode: Text.WordWrap
}
