import QtQuick 2.4
import FlightGear.Launcher 1.0
import "."


Item {

    Flickable {
        id: flick
        anchors {
            left: parent.left
            right: scrollbar.right
            top: parent.top
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

            text: _config.htmlForCommandLine();
        }
    }

    Scrollbar {
        id: scrollbar
        anchors.right: parent.right
        height: flick.height
        flickable: flick
        visible: flick.contentHeight > flick.height
    }
}
