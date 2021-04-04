import QtQuick 2.2
import FlightGear.Launcher 1.0 as FG
import FlightGear 1.0

Item {
    id: root
    height: visible ? contentBox.height + (Style.margin * 2) : 0
    z: 100

    property GettingStartedTipLayer tips
    property alias contents: contentBox.children

    Rectangle {
        id: contentBox
        width: parent.width - Style.strutSize * 2
        x: Style.strutSize
        height: Style.strutSize
        y: Style.margin
        color: Style.backgroundColor
        border.width: 1
        border.color: Style.minorFrameColor

    }

}
