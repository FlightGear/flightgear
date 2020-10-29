import QtQuick 2.0
import FlightGear.Launcher 1.0 as FG
import "."

Item {
    id: root

    property alias model: aircraftList.model
    property alias header: aircraftList.header
    property alias footer: aircraftList.footer

    signal showDetails(var uri);

    function updateSelectionFromLauncher()
    {
        if (!model)
            return;

        model.selectVariantForAircraftURI(_launcher.selectedAircraft);
        var row = model.indexForURI(_launcher.selectedAircraft);
        if (row >= 0) {
            // sequence here is necessary so progrommatic moves
            // are instant
            aircraftList.highlightMoveDuration = 0;
            aircraftList.currentIndex = row;
            aircraftList.highlightMoveDuration = aircraftList.__realHighlightMoveDuration;
        } else {
            // clear selection in view, so we don't show something
            // erroneous such as the previous value
            aircraftList.currentIndex = -1;
        }
    }

    onModelChanged: updateSelectionFromLauncher()

    Component {
        id: highlight
        Rectangle {
            gradient: Gradient {
                      GradientStop { position: 0.0; color: "#98A3B4" }
                      GradientStop { position: 1.0; color: "#5A6B83" }
                  }
        }
    }

    ScrolledListView {
        id: aircraftList

        anchors {
            left: parent.left
            top: parent.top
            bottom: parent.bottom
            right: scrollbar.left
            topMargin: Style.margin
        }

        delegate: AircraftCompactDelegate {
            onSelect: {
                aircraftList.currentIndex = model.index;
                _launcher.selectedAircraft = uri;
            }

            onShowDetails: root.showDetails(uri)
        }

        clip: true
        focus: true

        highlight: highlight
        highlightMoveDuration: __realHighlightMoveDuration

        // saved here becuase we need to reset highlightMoveDuration
        // when doing a progrmatic set
        readonly property int __realHighlightMoveDuration: 200
    }

    FGCompatScrollbar {
        id: scrollbar
        anchors.right: parent.right
        anchors.top: parent.top
        height: aircraftList.height
        flickable: aircraftList
    }
}
