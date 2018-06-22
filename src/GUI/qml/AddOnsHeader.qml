import QtQuick 2.4
import "."

Item {
    id: root

    property bool showAddButton: false

    property alias title: headerTitle.text
    property alias description: description.text
    signal add();

    implicitWidth: parent.width
    implicitHeight: headerRect.height + Style.margin + description.height

    Rectangle {
        id: headerRect
        width: parent.width
        height: headerTitle.height + (Style.margin * 2)

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

        AddButton {
            id: addButton
            visible: root.showAddButton
            anchors.right: parent.right
            anchors.rightMargin: Style.margin
            anchors.verticalCenter: parent.verticalCenter
            onClicked: root.add()
        }
    }

    StyledText {
        id: description
        width: parent.width
        anchors.top: headerRect.bottom
        anchors.topMargin: Style.margin
        wrapMode: Text.WordWrap
    }
}
