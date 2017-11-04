import QtQuick 2.0
import FlightGear 1.0 as FG

Rectangle {
    property bool uiVisible: true
    width: 1024
    height: 768
    color: "black"

    property double __uiOpacity: uiVisible ? 1.0 : 0.0
    property bool __uiVisible: true

    Behavior on __uiOpacity {
        SequentialAnimation {
            ScriptAction { script: if (uiVisible) __uiVisible = true; }
            NumberAnimation { duration: 250 }
            ScriptAction { script: if (!uiVisible) __uiVisible = false; }
        }
    }

    Image {
        opacity: __uiOpacity * 0.5
        source: "qrc:///images/checkerboard"
        fillMode: Image.Tile
        anchors.fill: parent
        visible: __uiVisible
    }

    MouseArea {
        anchors.fill: parent
        onClicked: {
            uiVisible = !uiVisible;
        }
    }

    Repeater {
        model: _application.activeCanvases
        delegate: CanvasFrame {
            id: display
            canvas: modelData
            showUi: __uiVisible
        }
    }

    BrowsePanel {
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 400
        visible: __uiVisible
        opacity: __uiOpacity
    }

    LoadSavePanel {
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        width: 400
        visible: __uiVisible
        opacity: __uiOpacity
    }
}
