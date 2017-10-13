import QtQuick 2.2
import FlightGear.Launcher 1.0 as FG

Item
{
    id: root

    readonly property int margin: 8

    Rectangle
    {
        id: tabBar
        height: installedAircraftButton.height + margin
        width: parent.width

        Row {
            anchors.centerIn: parent
            spacing: root.margin

            TabButton {
                id: installedAircraftButton
                text: qsTr("Installed Aircraft")
                onClicked: {
                    root.state = "installed"
                }
                active: root.state == "installed"
            }

            TabButton {
                id: browseButton
                text: qsTr("Browse")
                onClicked: {
                    root.state = "browse"
                }
                active: root.state == "browse"
            }
        } // of header row

        SearchButton {
            id: searchButton

            width: 180
            height: installedAircraftButton.height

            anchors.right: parent.right
            anchors.rightMargin: margin
            anchors.verticalCenter: parent.verticalCenter

            onSearch: {
               _launcher.searchAircraftModel.setAircraftFilterString(term)
                root.state = "search"
            }

            active: root.state == "search"
        }

        Rectangle {
            color: "#68A6E1"
            height: 1
            width: parent.width - 20
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
        }
    }


    Component {
        id: highlight
        Rectangle {
            gradient: Gradient {
                      GradientStop { position: 0.0; color: "#98A3B4" }
                      GradientStop { position: 1.0; color: "#5A6B83" }
                  }
        }
    }

    Component {
        id: ratingsHeader
        AircraftRatingsPanel {
            width: aircraftList.width - 80
            x: (aircraftList.width - width) / 2

        }
    }

    Component {
        id: noDefaultCatalogHeader
        NoDefaultCatalogPanel {
            width: aircraftList.width - 80
            x: (aircraftList.width - width) / 2
        }
    }

    Component {
        id: updateAllHeader
        UpdateAllPanel {
            width: aircraftList.width - 80
            x: (aircraftList.width - width) / 2
        }
    }

    ListView {
        id: aircraftList

        anchors {
            left: parent.left
            top: tabBar.bottom
            bottom: parent.bottom
            right: scrollbar.left
        }

        delegate: AircraftCompactDelegate {
            onSelect: {
                aircraftList.currentIndex = model.index;
                _launcher.selectedAircraft = uri;
            }

            onShowDetails: root.showDetails(uri)
        }

        clip: true

        highlight: highlight
        highlightMoveDuration: 100

        Keys.onUpPressed: {

        }

        Keys.onDownPressed: {

        }

        function updateSelectionFromLauncher()
        {
            model.selectVariantForAircraftURI(_launcher.selectedAircraft);
            var row = model.indexForURI(_launcher.selectedAircraft);
            if (row >= 0) {
                currentIndex = row;
            } else {
                // clear selection in view, so we don't show something
                // erroneous such as the previous value
                currentIndex = -1;
            }
        }
    }

    Scrollbar {
        id: scrollbar
        anchors.right: parent.right
        anchors.top: tabBar.bottom
        height: aircraftList.height
        flickable: aircraftList
    }

    Connections
    {
        target: _launcher
        onSelectAircraftIndex: {
            console.warn("Selecting aircraft:" + index);
            aircraftList.currentIndex = index;
            aircraftList.model.select
        }
    }

    state: "installed"

    states: [
        State {
            name: "installed"
            PropertyChanges {
                target: aircraftList
                model: _launcher.installedAircraftModel
                header: _launcher.baseAircraftModel.showUpdateAll ? updateAllHeader : null
            }
        },

        State {
            name: "search"
            PropertyChanges {
                target: aircraftList
                model: _launcher.searchAircraftModel
                header: null
            }
        },

        State {
            name: "browse"
            PropertyChanges {
                target: aircraftList
                model: _launcher.browseAircraftModel
                header: _launcher.showNoOfficialHanger ? noDefaultCatalogHeader : ratingsHeader
            }
        }
    ]

    function showDetails(uri)
    {
        // set URI, start animation
        // change state
        detailsView.aircraftURI = uri;
        detailsView.visible = true
    }

    function goBack()
    {
        detailsView.visible = false;
    }

    AircraftDetailsView {
        id: detailsView
        anchors.fill: parent
        visible: false

        Button {
            anchors { left: parent.left; top: parent.top; margins: root.margin }
            width: 60

            id: backButton
            text: "< Back"
            onClicked: {
                // ensure that if the variant was changed inside the detailsView,
                // that we update our selection correctly
                aircraftList.updateSelectionFromLauncher();
                root.goBack();
            }
        }
    }
}

