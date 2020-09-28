import QtQuick 2.0
import QtQuick.Controls 2.2

import FlightGear.Launcher 1.0 as FG

Item {
    id: root

    property alias model: view.model
    property alias header: view.header
    property alias footer: view.footer

    signal showDetails(var uri);

    function updateSelectionFromLauncher()
    {
        if (!model)
            return;

        var row = model.indexForURI(_launcher.selectedAircraft);
        if (row >= 0) {
            view.currentIndex = row;
        } else {
            // clear selection in view, so we don't show something
            // erroneous such as the previous value
            view.currentIndex = -1;
        }
    }

    onModelChanged: updateSelectionFromLauncher();

    Component {
        id: highlight
        Rectangle {
            gradient: Gradient {
                      GradientStop { position: 0.0; color: "#98A3B4" }
                      GradientStop { position: 1.0; color: "#5A6B83" }
                  }
        }
    }

    GridView {
        id: view
        cellWidth: width / colCount
        cellHeight: 128 + Style.strutSize
        highlightMoveDuration: 0
        
        ScrollBar.vertical: ScrollBar {}

        readonly property int baseCellWidth: 172 + (Style.strutSize * 2)
        readonly property int colCount: Math.floor(width / baseCellWidth)

        anchors {
            left: parent.left
            top: parent.top
            bottom: parent.bottom
            right: parent.right
            topMargin: Style.margin
        }

        delegate: AircraftGridDelegate {
            width: view.cellWidth
            height: view.cellHeight

            onSelect: {
                view.currentIndex = model.index;
                _launcher.selectedAircraft = uri;
            }

            onShowDetails: root.showDetails(uri)
        }

        clip: true
        focus: true
        highlight: highlight
    }
}
