import QtQuick 2.0
import FlightGear 1.0

CanvasItem {
    implicitHeight: img.implicitHeight
    implicitWidth: img.implicitWidth

    property alias source: img.source

    Image {
        id: img

    }
}
