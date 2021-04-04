import QtQuick 2.4
import QtQuick.Controls 2.2

import FlightGear.Launcher 1.0
import FlightGear 1.0

Item {

    Row {
        id: buttonRow
        spacing: Style.margin
        height: childrenRect.height
        anchors {
            margins: Style.margin
            top: parent.top
            left: parent.left
            right: parent.right
        }

        Button {
            text: qsTr("Copy to clipboard")
            onClicked: {
                _config.copyCommandLine();
            }
        }
    }

    Flickable {
        id: flick
        ScrollBar.vertical: ScrollBar {}

        anchors {
            left: parent.left
            right: parent.right
            top: buttonRow.bottom
            bottom: parent.bottom
            margins: Style.margin
        }

        contentHeight: contents.implicitHeight

        TextEdit {
            id: contents
            width: parent.width
            selectByMouse: true

            textFormat: TextEdit.RichText
            readOnly: true

            wrapMode: TextEdit.Wrap
            font.pixelSize: Style.baseFontPixelSize
            color: Style.baseTextColor

            text: _config.htmlForCommandLine();
        }
    }
}
