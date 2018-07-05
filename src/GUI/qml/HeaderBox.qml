import QtQuick 2.4
import "."

Rectangle {
    id: headerRect
    implicitHeight: headerTitle.height + (Style.margin * 2)

    property alias title: headerTitle.text

    color: Style.themeColor
    border.width: 1
    border.color: Style.frameColor

    Text {
        id: headerTitle
        color: "white"
        anchors.verticalCenter: parent.verticalCenter
        font.bold: true
        font.pixelSize: Style.subHeadingFontPixelSize
        anchors.left: parent.left
        anchors.leftMargin: Style.inset
    }
}
