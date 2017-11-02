import QtQuick 2.0
import FlightGear 1.0

CanvasItem {
    implicitHeight: text.implicitHeight
    implicitWidth: text.implicitWidth

    property alias text: text.text
    property alias fontPixelSize: text.font.pixelSize
    property alias fontFamily: text.font.family
    property alias color: text.color

    Text {
        id: text
        font.pixelSize: 20
        color: "yellow"
    }
}
