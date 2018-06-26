import QtQuick 2.4
import "."

BaseMenuItem {
    height: 3
    Rectangle {
        x: Style.margin
        y: 1
        width: parent.width - (2 * Style.margin)
        height: 1
        color: Style.themeColor
    }
}