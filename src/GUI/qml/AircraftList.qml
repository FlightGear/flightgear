import QtQuick 2.2
import FlightGear.Launcher 1.0 as FG
import "." // -> forces the qmldir to be loaded

FocusScope
{
    id: root

    Component.onCompleted: {
        aircraftList.updateSelectionFromLauncher();
    }

    Rectangle
    {
        id: tabBar
        height: searchButton.height + (Style.margin * 2)
        width: parent.width

        Row {
            anchors.centerIn: parent
            spacing: Style.margin

            TabButton {
                id: installedAircraftButton
                text: qsTr("Installed Aircraft")
                onClicked: {
                    root.state = "installed"
                    aircraftList.updateSelectionFromLauncher();
                }
                active: root.state == "installed"
            }

            TabButton {
                id: browseButton
                text: qsTr("Browse")
                onClicked: {
                    root.state = "browse"
                    aircraftList.updateSelectionFromLauncher();
                }
                active: root.state == "browse"
            }

            TabButton {
                id: updatesButton
              //  visible: _launcher.baseAircraftModel.showUpdateAll
                text: qsTr("Updates")
                onClicked: {
                    root.state = "updates"
                    aircraftList.updateSelectionFromLauncher();
                }
                active: root.state == "updates"
            }
        } // of header row

        SearchButton {
            id: searchButton

            height: installedAircraftButton.height

            anchors.right: parent.right
            anchors.rightMargin: Style.margin
            anchors.verticalCenter: parent.verticalCenter

            onSearch: {
               _launcher.searchAircraftModel.setAircraftFilterString(term)
                root.state = "search"
                aircraftList.updateSelectionFromLauncher();
            }

            active: root.state == "search"
        }
    }

    Rectangle {
        id: tabBarDivider
        color: Style.frameColor
        height: 1
        width: parent.width - Style.inset
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: tabBar.bottom
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
            width: aircraftList.width - Style.strutSize * 2
            x: (aircraftList.width - width) / 2
            theList: aircraftList
        }
    }

    Component {
        id: noDefaultCatalogHeader
        NoDefaultCatalogPanel {
            width: aircraftList.width - Style.strutSize * 2
            x: (aircraftList.width - width) / 2
        }
    }

    Component {
        id: updateAllHeader
        UpdateAllPanel {
            width: aircraftList.width - Style.strutSize * 2
            x: (aircraftList.width - width) / 2
        }
    }

    ListView {
        id: aircraftList

        anchors {
            left: parent.left
            top: tabBarDivider.bottom
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

        // prevent mouse wheel interactions when the details view is
        // visible, since it has its own flickable
        enabled: !detailsView.visible

        highlight: highlight
        highlightMoveDuration: __realHighlightMoveDuration

        // saved here becuase we need to reset highlightMoveDuration
        // when doing a progrmatic set
        readonly property int __realHighlightMoveDuration: 200

        function updateSelectionFromLauncher()
        {
            model.selectVariantForAircraftURI(_launcher.selectedAircraft);
            var row = model.indexForURI(_launcher.selectedAircraft);
            if (row >= 0) {
                // sequence here is necessary so progrommatic moves
                // are instant
                highlightMoveDuration = 0;
                currentIndex = row;
                highlightMoveDuration = __realHighlightMoveDuration;
            } else {
                // clear selection in view, so we don't show something
                // erroneous such as the previous value
                currentIndex = -1;
            }
        }
    }

    StyledText {
        id: noUpdatesMessage
        anchors {
            left: parent.left
            top: tabBar.bottom
            bottom: parent.bottom
            right: scrollbar.left
        }
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        font.pixelSize: Style.headingFontPixelSize
        text: qsTr("No aircraft updates available right now")
        visible: (root.state == "updates") && (_launcher.aircraftWithUpdatesModel.count == 0)
    }

    Scrollbar {
        id: scrollbar
        anchors.right: parent.right
        anchors.top: tabBar.bottom
        height: aircraftList.height
        flickable: aircraftList
    }

    state: "installed"

    states: [
        State {
            name: "installed"
            PropertyChanges {
                target: aircraftList
                model: _launcher.installedAircraftModel
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
                header: _addOns.showNoOfficialHangar ? noDefaultCatalogHeader : ratingsHeader
            }
        },

        State {
            name: "updates"
            PropertyChanges {
                target: aircraftList
                model: _launcher.aircraftWithUpdatesModel
                header: (_launcher.aircraftWithUpdatesModel.count > 0) ? updateAllHeader : null
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
            anchors { left: parent.left; top: parent.top; margins: Style.margin }
            width: Style.strutSize

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

