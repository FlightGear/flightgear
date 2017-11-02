import QtQuick 2.0
import FlightGear 1.0 as FG

Rectangle {
    property bool __uiVisible: true
    width: 1024
    height: 768
    color: "black"

    Image {
        opacity: visible ? 0.5 : 0.0
        source: "qrc:///images/checkerboard"
        fillMode: Image.Tile
        anchors.fill: parent
        visible: __uiVisible
        Behavior on opacity {
            NumberAnimation { duration: 400 }
        }
    }

    MouseArea {
        anchors.fill: parent
        onClicked: {
            __uiVisible = !__uiVisible;
        }
    }

    Repeater {
        model: _application.activeCanvases
        delegate: CanvasFrame {
            id: display
            canvas: modelData
        }
    }

    BrowsePanel {
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 400
        visible: __uiVisible
        opacity: visible ? 1.0: 0.0
        Behavior on opacity {
            NumberAnimation { duration: 400 }
        }
    }

    LoadSavePanel {
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        width: 400
        visible: __uiVisible
        opacity: visible ? 1.0: 0.0
        Behavior on opacity {
            NumberAnimation { duration: 400 }
        }
    }
}
