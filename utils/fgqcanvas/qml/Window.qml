import QtQuick 2.0

Rectangle {
    width: 1024
    height: 768
    color: "black"

    // only show the UI on the main window
    property double __uiOpacity: __shouldShowUi ? 1.0 : 0.0
    property bool __uiVisible: true
    readonly property bool __shouldShowUi: (isMainWindow && _application.showUI)
    readonly property bool isMainWindow: (_windowNumber === 0)

    Component.onCompleted: {
        // synchronize insitial state of this
        __uiVisible = __shouldShowUi;
    }

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

        // we use a loader to only create canvases on the correct window
        // by driving the 'active' property
        delegate: Loader {
            id: canvasLoader
            sourceComponent: canvasFrame
            active: modelData.windowIndex === _windowNumber

            Binding {
                target: canvasLoader.item
                property: "canvas"
                value: model.modelData
            }
        }
    }

    Component {
        id: canvasFrame
        CanvasFrame {
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

    GetStarted {
        visible: isMainWindow
        anchors.centerIn: parent
    }


}
