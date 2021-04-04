import QtQuick 2.4
import FlightGear.Launcher 1.0 as FG
import FlightGear 1.0

Rectangle {
    id: addCatalogPanel
    state: "start"
    height: column.height
    color: Style.panelBackground

    readonly property bool isActive: (state != "start")
    Behavior on height {
        enabled: false // disable to prevent animation on load
        id: heightBehaviour
        NumberAnimation { duration: 200; }
    }

    Timer { // ugly: timer to enable the animation after loading is done
        onTriggered: heightBehaviour.enabled = true;
        running: true
        interval: 5
    }

    Column {
        id: column
        width: parent.width
        spacing: Style.margin

        ClickableText {
            id: addCatalogPromptText

            width: parent.width
            wrapMode: Text.WordWrap

            onClicked: {
                addCatalogPanel.state = "enter-url";
            }
        }

        LineEdit {
            id: addCatalogUrlEntry
            width: parent.width
            useFullWidth: true
            label: qsTr("Hangar URL:");
            placeholder: "http://www.flightgear.org/hangars/some-catalog.xml"
        }

        AnimatedImage {
            id: spinner
            anchors.horizontalCenter: parent.horizontalCenter
            source: "qrc:///linear-spinner"
            playing: true
        }

        Item {
            id: buttonBox
            width: parent.width
            height: childrenRect.height

            Button {
                id: addCatalogCancelButton
                anchors.right: addCatalogProceedButton.left
                anchors.rightMargin: Style.margin

                text: qsTr("Cancel")

                onClicked:  {
                    _addOns.catalogs.abandonAddCatalog();
                    addCatalogPanel.state = "start";
                }
            }

            Button {
                id: addCatalogProceedButton
                anchors.right: parent.right

                text: qsTr("Add hangar")

                // todo check for a well-formed URL
                enabled: addCatalogUrlEntry.text !== ""

                onClicked:  {
                    // this occurs in the retry case; abandon the current
                    // add before starting the new one
                    if (_addOns.catalogs.isAddingCatalog) {
                        _addOns.catalogs.abandonAddCatalog();
                    }

                    _addOns.catalogs.addCatalogByUrl(addCatalogUrlEntry.text)
                    addCatalogPanel.state = "add-in-progress"
                }
            }
        } // of button box
    } // of column layout

    Connections {
        target: _addOns.catalogs
        onStatusOfAddingCatalogChanged: {
            var status = _addOns.catalogs.statusOfAddingCatalog;
            if (status == FG.CatalogListModel.Refreshing) {
                // this can happen when adding the default catalog,
                // ensure our state matches
                addCatalogPanel.state = "add-in-progress"
                return;
            }

            if (status == FG.CatalogListModel.NoAddInProgress) {
                addCatalogPanel.state = "start";
                return;
            }

            if (status == FG.CatalogListModel.Ok) {
                // success, we are done
                _addOns.catalogs.finalizeAddCatalog();
                addCatalogPanel.state = "start";
                return;
            }

            // if we reach this point, we're in an error situation, so
            // handle that
            addCatalogPanel.state = "add-failed-retry";
        }
    }

    function addErrorMessage()
    {
        switch (_addOns.catalogs.statusOfAddingCatalog) {
        case FG.CatalogListModel.NotFoundOnServer:
            return qsTr("Failed to find a hangar description at the URL: '%1'. " +
                        "Check you entered the URL correctly.");
        case FG.CatalogListModel.HTTPForbidden:
            return qsTr("Access to the hangar data was forbidden by the server. " +
                        "Please check the URL you entered, or contact the hangar authors.");
        case FG.CatalogListModel.NetworkError:
            return qsTr("Failed to download from the server due to a network problem. " +
                        "Check your Internet connection is working, and that you entered the correct URL.");
        case FG.CatalogListModel.IncomaptbleVersion:
            return qsTr("The hangar you requested is for a different version of FlightGear. "
                       + "(This is version %1)").arg(_launcher.versionString);

        case FG.CatalogListModel.InvalidData:
            return qsTr("The requested URL doesn't contain valid hangar data. "
                        + "Check you entered a valid hangar URL. If it's correct, "
                        + "please contact the hangar authors, or try again later." );

        default:
            return qsTr("Unknown error: " + _addOns.catalogs.statusOfAddingCatalog);
        }
    }

    states: [
        State {
            name: "start"
            PropertyChanges { target: addCatalogPromptText; text: qsTr("Click here to add a new aircraft hangar. (Note this requires an Internet connection)"); clickable: true }
            PropertyChanges { target: addCatalogUrlEntry; visible: false; text: "" } // reset URL on cancel / success
            PropertyChanges { target: buttonBox; visible: false }
            PropertyChanges { target: spinner; visible: false }
        },

        State {
            name: "enter-url"
            PropertyChanges { target: addCatalogPromptText; text: qsTr("Enter a hangar location (URL) to add."); clickable: false }
            PropertyChanges { target: addCatalogUrlEntry; visible: true }
            PropertyChanges { target: buttonBox; visible: true }
            PropertyChanges { target: spinner; visible: false }
        },

        State {
            name: "add-in-progress"
            PropertyChanges { target: addCatalogPromptText; text: qsTr("Retrieving hangar information..."); clickable: false }
            PropertyChanges { target: addCatalogUrlEntry; visible: false }
            PropertyChanges { target: addCatalogProceedButton; enabled: false }
            PropertyChanges { target: spinner; visible: true }
        },


        State {
            name: "add-failed-retry"
            PropertyChanges { target: addCatalogPromptText; text: qsTr("There was a problem adding the hangar: %1.").arg(addErrorMessage()); clickable: false }
            PropertyChanges { target: addCatalogUrlEntry; visible: true }
            PropertyChanges { target: addCatalogProceedButton; enabled: true }
            PropertyChanges { target: spinner; visible: false }
        }
    ]
}
