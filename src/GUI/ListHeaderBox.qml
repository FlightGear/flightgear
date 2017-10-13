import QtQuick 2.2
import FlightGear.Launcher 1.0 as FG

Item {
    id: root
    readonly property int margin: 8

    height: visible ? 48 : 0
    z: 100

    property alias contents: contentBox.children

    Rectangle {
        id: contentBox
        width: parent.width
        height: 40
        y: 4
        color: "white"
        border.width: 1
        border.color: "#9f9f9f"



    }

}
