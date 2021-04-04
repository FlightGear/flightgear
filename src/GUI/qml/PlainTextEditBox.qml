import QtQuick 2.4
import FlightGear.Launcher 1.0
import FlightGear 1.0

Rectangle {
    id: editFrame

    property bool enabled: true
    property alias placeholder: placeholderText.text
    property alias text: edit.text

    implicitHeight: edit.height + Style.margin
    border.color: edit.activeFocus ? Style.frameColor : Style.minorFrameColor
    border.width: 1
    color: Style.panelBackground
    
    signal editingFinished();

    TextEdit {
        id: edit
        enabled: editFrame.enabled
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: Style.margin
        height: Math.max(Style.strutSize * 2, implicitHeight)
        textFormat: TextEdit.PlainText
        font.family: "Courier"
        selectByMouse: true
        wrapMode: TextEdit.WordWrap
        font.pixelSize: Style.baseFontPixelSize
        color: Style.baseTextColor

        StyledText {
            id: placeholderText
            visible: (edit.text.length == 0) && !edit.activeFocus
            font.family: "Courier"
            color: Style.disabledTextColor
        }

        onActiveFocusChanged: {
            if (!activeFocus) {
                editFrame.editingFinished()
            }
        }
   }
}
