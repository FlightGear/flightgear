import QtQuick 2.4
import FlightGear.Launcher 1.0
import "."

Flickable {
    flickableDirection: Flickable.VerticalFlick
    contentHeight: contents.childrenRect.height

    interactive: false

    Component {
        id: catalogDelegate

        Rectangle {
            id: delegateRoot

            // don't show the delegate for newly added catalogs, until the
            // adding process is complete
            visible: !model.isNewlyAdded

            height: catalogTextColumn.childrenRect.height + Style.margin * 2
            border.width: 1
            border.color: Style.themeColor
            width: catalogsColumn.width


            Column {
                id: catalogTextColumn

                y: Style.margin
                anchors.left: parent.left
                anchors.leftMargin: Style.margin
                anchors.right: catalogDeleteButton.left
                anchors.rightMargin: Style.margin
                spacing: Style.margin

                Text {
                    font.pixelSize: Style.subHeadingFontPixelSize
                    font.bold: true
                    width: parent.width
                    text: model.name
                }

                Text {
                    visible: model.status === CatalogListModel.Ok
                    width: parent.width
                    text: model.description
                    wrapMode: Text.WordWrap
                }

                ClickableText {
                    visible: model.status !== CatalogListModel.Ok
                    width: parent.width
                    wrapMode: Text.WordWrap
                    text: qsTr("This hangar is currently disabled due to a problem. " +
                               "Click here to try updating the hangar information from the server. "
                               + "(An Internet connection is required for this)");
                    onClicked:  {
                        _addOns.catalogs.refreshCatalog(model.index)
                    }
                }

                Text {
                    width: parent.width
                    text: model.url
                }

            }

            DeleteButton {
                id: catalogDeleteButton
                anchors.right: parent.right
                anchors.rightMargin: Style.margin
                anchors.verticalCenter: parent.verticalCenter
                visible: delegateHover.containsMouse

                onClicked:  {
                    confirmDeleteHangar.visible = true;
                }
            }

            MouseArea {
                id: delegateHover
                anchors.fill: parent
                hoverEnabled: true
                acceptedButtons: Qt.NoButton
            }

            YesNoPanel {
                id: confirmDeleteHangar
                visible: false
                anchors.fill: parent

                yesText: qsTr("Remove")
                noText: qsTr("Cancel")
                yesIsDestructive: true
                promptText: qsTr("Remove this hangar? (Downloaded aircraft will be deleted from your computer)");
                onAccepted: {
                    _addOns.catalogs.removeCatalog(model.index);
                }

                onRejected: {
                    confirmDeleteHangar.visible = false
                }
            }


        }
    }

    Column {
        id: contents
        width: parent.width - (Style.margin * 2)
        x: Style.margin
        spacing: Style.margin

//////////////////////////////////////////////////////////////////
// catalogs //////////////////////////////////////////////////////

        Item {
            id: catalogHeaderItem
            width: parent.width
            height: catalogHeadingText.height + catalogDescriptionText.height + Style.margin

            Text {
                id: catalogHeadingText
                text: qsTr("Aircraft hangars")
                font.pixelSize: Style.headingFontPixelSize
                width: parent.width
            }

            Text {
                id: catalogDescriptionText
                text: qsTr("Aircraft hangars are managed collections of aircraft, which can be " +
                           "downloaded, installed and updated inside FlightGear.")
                anchors {
                    top: catalogHeadingText.bottom
                    topMargin: Style.margin
                }

                width: parent.width
                wrapMode: Text.WordWrap
            }


        } // of catalogs header item

        Rectangle {
            width: parent.width
            height: catalogsColumn.childrenRect.height + Style.margin * 2
            border.width: 1
            border.color: Style.frameColor
            clip: true

            Column {
                id: catalogsColumn
                width: parent.width - Style.margin * 2
                x: Style.margin
                y: Style.margin
                spacing: Style.margin

                Repeater {
                    id: catalogsRepeater
                    model: _addOns.catalogs
                    delegate: catalogDelegate
                }

                ClickableText {
                    visible: !_addOns.isOfficialHangarRegistered && !addCatalogPanel.isActive
                    width: parent.width
                    text : qsTr("The official FlightGear aircraft hangar is not set up. To add it, click here.");
                    onClicked:  {
                        _addOns.catalogs.installDefaultCatalog()
                    }
                }

                AddCatalogPanel {
                    id: addCatalogPanel
                    width: parent.width
                }
            }
        }

//////////////////////////////////////////////////////////////////

        Item {
            // spacing item
            width: parent.width
            height: Style.margin * 2
        }

        Item {
            id: aircraftHeaderItem
            width: parent.width
            height: aircraftHeading.height + aircraftDescriptionText.height + Style.margin

            Text {
                id: aircraftHeading
                text: qsTr("Additional aircraft folders")
                font.pixelSize: Style.headingFontPixelSize

                anchors {
                    left: parent.left
                    right: addAircraftPathButton.left
                    rightMargin: Style.margin
                }
            }

            Text {
                id: aircraftDescriptionText
                text: qsTr("To use aircraft you download yourself, FlightGear needs to " +
                           "know the folder(s) containing the aircraft data.")
                anchors {
                    top: aircraftHeading.bottom
                    topMargin: Style.margin
                    left: parent.left
                    right: addAircraftPathButton.left
                    rightMargin: Style.margin
                }
                wrapMode: Text.WordWrap
            }

            AddButton {
                id: addAircraftPathButton
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                onClicked: {
                    var newPath =_addOns.addAircraftPath();
                    if (newPath != "") {
                        _addOns.aircraftPaths.push(newPath)
                    }
                }
            }
        } // of aircraft header item

        Rectangle {
            width: parent.width
            height: aircraftPathsColumn.childrenRect.height + 1
            border.width: 1
            border.color: Style.frameColor
            clip: true

            Column {
                id: aircraftPathsColumn
                width: parent.width - Style.margin * 2
                x: Style.margin

                Repeater {
                    id: aircraftPathsRepeater
                    model: _addOns.aircraftPaths
                    delegate: PathListDelegate {
                        width: aircraftPathsColumn.width
                        deletePromptText: qsTr("Remove the aircraft folder: '%1' from the list? (The folder contents will not be changed)").arg(modelData);

                        onPerformDelete: {
                            var modifiedPaths = _addOns.aircraftPaths.slice()
                            modifiedPaths.splice(model.index, 1);
                            _addOns.aircraftPaths = modifiedPaths;
                        }

                        onPerformMove: {
                            var modifiedPaths = _addOns.aircraftPaths.slice()
                            modifiedPaths.splice(model.index, 1);
                            modifiedPaths.splice(newIndex, 0, modelData)
                            _addOns.aircraftPaths = modifiedPaths;
                        }
                    }
                }

                Text {
                    visible: (aircraftPathsRepeater.count == 0)
                    width: parent.width
                    text : qsTr("No custom aircraft paths are configured.");
                }
            }


        }

//////////////////////////////////////////////////////////////////

        Item {
            // spacing item
            width: parent.width
            height: Style.margin * 2
        }

        Item {
            id: sceneryHeaderItem
            width: parent.width
            height: sceneryHeading.height + sceneryDescriptionText.height + Style.margin

            Text {
                id: sceneryHeading
                text: qsTr("Additional scenery folders")
                font.pixelSize: Style.headingFontPixelSize

                anchors {
                    left: parent.left
                    right: addSceneryPathButton.left
                    rightMargin: Style.margin
                }
            }

            Text {
                id: sceneryDescriptionText
                text: qsTr("To use scenery you download yourself, FlightGear needs " +
                           "to know the folders containing the scenery data. " +
                           "Adjust the order of the list to control which scenery is used in a region.");
                anchors {
                    top: sceneryHeading.bottom
                    topMargin: Style.margin
                    left: parent.left
                    right: addSceneryPathButton.left
                    rightMargin: Style.margin
                }
                wrapMode: Text.WordWrap
            }

            AddButton {
                id: addSceneryPathButton
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                onClicked: {
                    var newPath =_addOns.addSceneryPath();
                    if (newPath !== "") {
                        _addOns.sceneryPaths.push(newPath)

                    }
                }
            }
        } // of aircraft header item

        Rectangle {
            width: parent.width
            height: sceneryPathsColumn.childrenRect.height + 1
            border.width: 1
            border.color: Style.frameColor
            clip: true

            Column {
                id: sceneryPathsColumn
                width: parent.width - Style.margin * 2
                x: Style.margin

                Repeater {
                    id: sceneryPathsRepeater
                    model: _addOns.sceneryPaths

                    delegate: PathListDelegate {
                        width: sceneryPathsColumn.width
                        deletePromptText: qsTr("Remove the scenery folder: '%1' from the list? (The folder contents will not be changed)").arg(modelData);

                        onPerformDelete: {
                            var modifiedPaths = _addOns.sceneryPaths.slice()
                            modifiedPaths.splice(model.index, 1);
                            _addOns.sceneryPaths = modifiedPaths;
                        }

                        onPerformMove: {

                        }
                    }
                }

                Text {
                    visible: (sceneryPathsRepeater.count == 0)
                    width: parent.width
                    text : qsTr("No custom scenery paths are configured.");
                }
            }
        }

        Item {
            width: parent.width
            height: Math.max(installTarballText.implicitHeight, installTarballButton.height)
            Button {
                id: installTarballButton
                text: "Install add-on scenery"

                onClicked: {
                    var path = _addOns.installCustomScenery();
                    if (path != "") {
                        // insert into scenery paths if not already present
                        for (var p in _addOns.sceneryPaths) {
                            if (p === path)
                                return; // found, we are are done
                        }

                        // not found, add it
                        _addOns.sceneryPaths.push(path);
                    }
                }
            }

            Text {
                id: installTarballText
                anchors {
                    left: installTarballButton.right
                    right: parent.right
                    leftMargin: Style.margin
                }

                wrapMode: Text.WordWrap
                text: qsTr("If you have downloaded scenery manually from the official FlightGear website, " +
                           "you can use this button to extract and install it into a suitable folder. " +
                           "(Scenery downloaded this way should have a file name such as 'w40n020.tar.gz')"
                           )
            }
        } // of install-tarbal item
    } // of column

}
