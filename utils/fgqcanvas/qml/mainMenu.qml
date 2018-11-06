import QtQuick 2.0
import FlightGear 1.0 as FG

Rectangle {
    width: 1024
    height: 768
    color: "black"

    property double __uiOpacity: _application.showUI ? 1.0 : 0.0
    property bool __uiVisible: true

    Behavior on __uiOpacity {
        SequentialAnimation {
            ScriptAction { script: if (_application.showUI) __uiVisible = true; }
            NumberAnimation { duration: 400 }
            ScriptAction { script: if (!_application.showUI) __uiVisible = false; }
        }
    }

    Image {
        opacity: __uiOpacity * 0.5
        source: "qrc:///images/checkerboard"
        fillMode: Image.Tile
        anchors.fill: parent
        visible: __uiVisible
    }

    Repeater {
        model: _application.activeCanvases
        delegate: CanvasFrame {
            id: display
            canvas: modelData
            showUi: __uiVisible
        }
    }

    VerticalTabPanel {
        anchors.fill: parent
        tabs: [browsePanel, configPanel, snapshotsPanel]
        titles: ["Connect", "Load / Save", "Snapshots"]
        visible: __uiVisible
        opacity: __uiOpacity
    }

    Component {
        id: browsePanel
        BrowsePanel { }
    }

    Component {
        id: configPanel
        LoadSavePanel { }
    }

    Component {
        id: snapshotsPanel
        SnapshotsPanel { }
    }



}
