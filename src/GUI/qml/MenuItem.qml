import QtQuick 2.4
import "."

BaseMenuItem {
    id: root
    
    property alias text: itemText.text
    property alias shortcut: shortcutText.text

    signal triggered();

    function minWidth() { 
        return itemText.width + shortcutText.width + (Style.inset * 2)
    }

    Rectangle {
        height: parent.height
        width: parent.width
        visible: mouse.containsMouse
        color: "#cfcfcf"
    }

    Text {
        id: itemText
        font.pixelSize: Style.baseFontPixelSize
        color: mouse.containsMouse ? Style.themeColor :
            (root.enabled ? Style.baseTextColor : Style.disabledTextColor);

        anchors {
            left: parent.left
            leftMargin: Style.inset
            verticalCenter: parent.verticalCenter
        }
    }

    Text {
        id: shortcutText
        color: Style.disabledTextColor
        font.pixelSize: Style.baseFontPixelSize
        width: implicitWidth + Style.inset
        horizontalAlignment: Text.AlignRight

        anchors {
            right: parent.right
            rightMargin: Style.inset
            verticalCenter: parent.verticalCenter
        }
    }

    MouseArea {
        id: mouse
        enabled: root.enabled
        anchors.fill: parent
        hoverEnabled: root.enabled
        onClicked: {
            root.closeMenu();
            root.triggered();
        }
    }
}