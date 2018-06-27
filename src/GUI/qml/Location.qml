import QtQuick 2.4
import FlightGear 1.0
import FlightGear.Launcher 1.0
import "."

Item {
    id: root

    property bool __searchActive: false
    property string lastSearch

    function backToSearch()
    {
        detailLoader.sourceComponent = null
        _location.setBaseLocation(null)
        if (!_location.searchModel.haveExistingSearch) {
            _location.showHistoryInSearchModel();
        }
    }

    function selectLocation(guid, type)
    {
        selectedLocation.guid = guid;
        _location.setBaseLocation(selectedLocation)
        _location.addToRecent(selectedLocation);

        if (selectedLocation.isAirportType) {
            detailLoader.sourceComponent = airportDetails
        } else {
            detailLoader.sourceComponent = navaidDetails
        }
    }

    Component.onCompleted: {
        // important so we can leave the location page and return to it,
        // preserving the state
        if (_location.base.valid) {
            selectedLocation.guid = _location.base.guid;

            if (selectedLocation.isAirportType) {
                detailLoader.sourceComponent = airportDetails
            } else {
                detailLoader.sourceComponent = navaidDetails
            }
        } else if (_location.isBaseLatLon) {
            detailLoader.sourceComponent = navaidDetails
        } else {
            _location.showHistoryInSearchModel();
        }
    }

    Positioned {
        id: selectedLocation
    }

    Component {
        id: airportDetails
        LocationAirportView {
            id: airportView
        }
    }

    Component {
        id: navaidDetails
        LocationNavaidView {
            id: navaidView
        }
    }

    Rectangle {
        anchors.fill: parent
        color: "white"
    }

    Component {
        id: locationSearchDelegate
        Rectangle {
            id: delegateRoot
            height: delegateContent.height + Style.margin
            width: searchView.width

            function itemDescription()
            {
                if (model.type === Positioned.Fix) return model.ident

                if (model.type === Positioned.VOR) {
                    var freq = (model.frequency / 100).toFixed(3);
                    return "%1 - %2 (%3 MHz)".arg(model.ident).arg(model.name).arg(freq);
                }

                // general case
                return "%1 - %2".arg(model.ident).arg(model.name);
            }


            Item {
                id: delegateContent
                height: Math.max(delegateIcon.height, delegateText.height)
                width: parent.width

                Rectangle {
                    visible: delegateMouse.containsMouse
                    color: "#cfcfcf"
                }

                PixmapImage {
                    id: delegateIcon
                    anchors.left: parent.left
                    anchors.leftMargin: Style.margin
                    anchors.verticalCenter: parent.verticalCenter
                    image: model.icon
                }

                StyledText {
                    id: delegateText
                    anchors.right: parent.right
                    anchors.left: delegateIcon.right
                    anchors.rightMargin: Style.margin
                    anchors.leftMargin: Style.margin
                    anchors.verticalCenter: parent.verticalCenter
                    text: delegateRoot.itemDescription();
                    wrapMode: Text.WordWrap
                }

                MouseArea {
                    id: delegateMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked:  {
                        root.selectLocation(model.guid, model.type);
                    }
                }
            }

            Item {
                id: footer
                height: Style.margin
                width: parent.width
                anchors.bottom: parent.bottom

                Rectangle {
                    color: Style.frameColor
                    height: 1
                    width: parent.width - Style.strutSize
                    anchors.centerIn: parent
                }
            }
        }
    }

    Text {
        id: headerText
        text: qsTr("Location")
        font.pixelSize: Style.headingFontPixelSize
        anchors.left: parent.left
        anchors.leftMargin: Style.inset
        anchors.top: parent.top
        anchors.topMargin: Style.margin
    }

    SearchButton {
        id: searchButton

        anchors.right: parent.right
        anchors.top: headerText.bottom
        anchors.left: parent.left
        anchors.margins: Style.margin

        autoSubmit: false
        placeholder: qsTr("Search for an airport or navaid");

        onSearch: {
            // when th search term is cleared, show the history
            if (term == "") {
                _location.showHistoryInSearchModel();
                return;
            }

            var geod = _location.parseStringAsGeod(term)
            if (geod.valid) {
                _location.baseGeod = geod
                selectedLocation.guid = 0;
                detailLoader.sourceComponent = navaidDetails
                return;
            }

            root.lastSearch = term;
            _location.searchModel.setSearch(term, _launcher.aircraftType)
        }
    }

    StyledText {
        id: searchHelpText
        anchors.right: parent.right
        anchors.top: searchButton.bottom
        anchors.left: parent.left
        anchors.margins: Style.margin
        wrapMode: Text.WordWrap

        text: qsTr("Enter the name, partial name or ident of a navaid or fix, or an " +
                   "airport name or ICAO identifier. Alternatively, enter a latitude & longitude: for " +
                   "example 53.4,-3.4 or 18.4S, 87.23W")
    }

    Rectangle {
        id: headerSplit
        color: Style.frameColor
        height: 1
        width: parent.width - Style.inset
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: searchHelpText.bottom
        anchors.topMargin: Style.margin
    }

    ListView {
        id: searchView
        anchors.top: headerSplit.bottom
        anchors.topMargin: Style.margin
        width: parent.width
        anchors.bottom: parent.bottom
        model: _location.searchModel
        delegate: locationSearchDelegate
        clip: true

        header: Item {
            visible: _location.searchModel.isSearchActive
            width: parent.width
            height: visible ? 50 : 0

            Text {
                text: qsTr("Searching")
                anchors.verticalCenter: parent.verticalCenter
                anchors.right: parent.horizontalCenter
            }

            AnimatedImage {
                source: "qrc://linear-spinner"
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.horizontalCenter
            }

        }

        footer: Item {
            width: parent.width
            height: noResultsText.height
            visible: (parent.count === 0) && !_location.searchModel.isSearchActive
            Text {
                id: noResultsText
                width: parent.width
                text: qsTr("No results for found search '%1'").arg(root.lastSearch)
                wrapMode: Text.WordWrap
            }
        }
    }

    // scrollbar

    Loader {
        id: detailLoader
        anchors.fill: parent
        visible: sourceComponent != null

        onStatusChanged: {
            if (status == Loader.Ready) {
                if (selectedLocation.valid) {
                    item.location = selectedLocation.guid
                } else {
                    // lon-lat
                    item.geod = _location.baseGeod
                }
            }
        }
    }

    Button {
        id: backButton

        anchors { left: parent.left; top: parent.top; margins: Style.margin }
        width: Style.strutSize
        visible: detailLoader.visible
        text: "< Back"
        onClicked: root.backToSearch();
    }
}
