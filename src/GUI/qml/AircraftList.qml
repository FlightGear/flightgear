import QtQuick 2.2
import FlightGear.Launcher 1.0 as FG
import "." // -> forces the qmldir to be loaded

FocusScope
{
    id: root

    property var __model: null
    property Component __header: null
    property string __lastState: "installed"

    function updateSelectionFromLauncher()
    {
        if (aircraftContent.item) {
            aircraftContent.item.updateSelectionFromLauncher();
        }
    }

    Component.onCompleted: {
        _launcher.browseAircraftModel.loadRatingsSettings();
    }

    Rectangle
    {
        id: tabBar
        height: searchButton.height + (Style.margin * 2)
        width: parent.width

        GridToggleButton {
            id: gridModeToggle
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin: Style.margin
            gridMode: !_launcher.aircraftGridMode
            onClicked: _launcher.aircraftGridMode = !_launcher.aircraftGridMode
        }

        Row {
            anchors.centerIn: parent
            spacing: Style.margin

            TabButton {
                id: installedAircraftButton
                text: qsTr("Installed Aircraft")
                onClicked: {
                    root.state = "installed"
                    root.updateSelectionFromLauncher();
                }
                active: root.state == "installed"
            }

            TabButton {
                id: browseButton
                text: qsTr("Browse")
                onClicked: {
                    root.state = "browse"
                    root.updateSelectionFromLauncher();
                }
                active: root.state == "browse"
            }

            TabButton {
                id: updatesButton
              //  visible: _launcher.baseAircraftModel.showUpdateAll
                text: qsTr("Updates")
                onClicked: {
                    root.state = "updates"
                    root.updateSelectionFromLauncher();
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
                root.updateSelectionFromLauncher();
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
        id: ratingsHeader
        AircraftRatingsPanel {
            width: aircraftContent.width
            onClearSelection: {
                _launcher.selectedAircraft = "";
                root.updateSelectionFromLauncher()
            }
        }
    }

    Component {
        id: noDefaultCatalogHeader
        NoDefaultCatalogPanel {
            width: aircraftContent.width
        }
    }

    Component {
        id: updateAllHeader
        UpdateAllPanel {
            width: aircraftContent.width
        }
    }

    Component {
        id: emptyHeader
        Item {
        }
    }

    Loader {
        id: aircraftContent
        // we use gridModeToggle vis to mean enabled, effectively
        source: (gridModeToggle.visible && _launcher.aircraftGridMode) ? "qrc:///qml/AircraftGridView.qml"
                                           : "qrc:///qml/AircraftListView.qml"

        anchors {
            left: parent.left
            top: tabBarDivider.bottom
            bottom: parent.bottom
            right: parent.right
            topMargin: Style.margin
        }

        Binding {
            target: aircraftContent.item
            property: "model"
            value: root.__model
        }

        Binding {
            target: aircraftContent.item
            property: "header"
            value: root.__header
        }

        Connections {
            target: aircraftContent.item
            onShowDetails: root.showDetails(uri)
        }
    }

    StyledText {
        id: noUpdatesMessage
        anchors {
            left: parent.left
            top: tabBar.bottom
            bottom: parent.bottom
            right: parent.right
        }
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        font.pixelSize: Style.headingFontPixelSize
        text: qsTr("No aircraft updates available right now")
        visible: (root.state == "updates") && (_launcher.aircraftWithUpdatesModel.count == 0)
    }

    state: "installed"

    states: [
        State {
            name: "installed"
            PropertyChanges {
                target: root
                __model: _launcher.installedAircraftModel
                __header: emptyHeader
            }

            PropertyChanges {
                target: gridModeToggle; visible: true
            }
        },

        State {
            name: "search"
            PropertyChanges {
                target: root
                __model: _launcher.searchAircraftModel
                __header: emptyHeader
            }

            PropertyChanges {
                target: gridModeToggle; visible: true
            }
        },

        State {
            name: "browse"
            PropertyChanges {
                target: root
                __model: _launcher.browseAircraftModel
                __header: _addOns.showNoOfficialHangar ? noDefaultCatalogHeader : ratingsHeader
            }

            PropertyChanges {
                target: gridModeToggle; visible: true
            }
        },

        State {
            name: "updates"
            PropertyChanges {
                target: root
                __model: _launcher.aircraftWithUpdatesModel
                __header: (_launcher.aircraftWithUpdatesModel.count > 0) ? updateAllHeader : emptyHeader
            }

            PropertyChanges {
                target: gridModeToggle; visible: false
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
        // details view can change the aircraft URI / variant
        updateSelectionFromLauncher();
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
            onClicked: root.goBack();
        }
    }
}

