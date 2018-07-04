import QtQuick 2.4
import FlightGear.Launcher 1.0
import "."

Item {
    Flickable {
        id: flick
        height: parent.height
        width: parent.width - scrollbar.width
        flickableDirection: Flickable.VerticalFlick
        contentHeight: contents.childrenRect.height

        Column {
            id: contents
            width: parent.width - (Style.margin * 2)
            x: Style.margin
            spacing: Style.margin

    //////////////////////////////////////////////////////////////////
    // catalogs //////////////////////////////////////////////////////

            AddOnsHeader {
                id: catalogHeader
                title: qsTr("Aircraft hangars")
                description: qsTr("Aircraft hangars are managed collections of aircraft, which can be " +
                                  "downloaded, installed and updated inside FlightGear.")
            }

            Rectangle {
                width: parent.width
                height: catalogsColumn.childrenRect.height + Style.margin * 2
                border.width: 1
                border.color: Style.frameColor
                clip: true

                Column {
                    id: catalogsColumn
                    width: parent.width
                    spacing: Style.margin

                    Repeater {
                        id: catalogsRepeater
                        model: _addOns.catalogs
                        delegate: CatalogDelegate {
                            width: catalogsColumn.width
                        }
                    }

                    ClickableText {
                        visible: !_addOns.isOfficialHangarRegistered && !addCatalogPanel.isActive
                        anchors { left: parent.left; right: parent.right; margins: Style.margin }
                        text : qsTr("The official FlightGear aircraft hangar is not set up. To add it, click here.");
                        onClicked:  {
                            _addOns.catalogs.installDefaultCatalog()
                        }
                    }

                    AddCatalogPanel {
                        id: addCatalogPanel
                        anchors { left: parent.left; right: parent.right; margins: Style.margin }
                    }
                }
            }

    //////////////////////////////////////////////////////////////////

            Item {
                // spacing item
                width: parent.width
                height: Style.margin * 2
            }

            AddOnsHeader {
                id: aircraftHeader
                title: qsTr("Additional aircraft folders")
                description: qsTr("To use aircraft you download yourself, FlightGear needs to " +
                                  "know the folder(s) containing the aircraft data.")
                showAddButton: true
                onAdd: {
                    var newPath =_addOns.addAircraftPath();
                    if (newPath !== "") {
                        _addOns.aircraftPaths.push(newPath)
                    }
                }
            }

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
                            modelCount: _addOns.aircraftPaths.length

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

                    StyledText {
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

            AddOnsHeader {
                id: sceneryHeader
                title: qsTr("Additional scenery folders")
                description: qsTr("To use scenery you download yourself, FlightGear needs " +
                                  "to know the folders containing the scenery data. " +
                                  "Adjust the order of the list to control which scenery is used in a region.");
                showAddButton: true
                onAdd: {
                    var newPath =_addOns.addSceneryPath();
                    if (newPath !== "") {
                        _addOns.sceneryPaths.push(newPath)
                    }
                }
            }

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
                            modelCount: _addOns.sceneryPaths.length

                            onPerformDelete: {
                                var modifiedPaths = _addOns.sceneryPaths.slice()
                                modifiedPaths.splice(model.index, 1);
                                _addOns.sceneryPaths = modifiedPaths;
                            }

                            onPerformMove: {
                                var modifiedPaths = _addOns.sceneryPaths.slice()
                                modifiedPaths.splice(model.index, 1);
                                modifiedPaths.splice(newIndex, 0, modelData)
                                _addOns.sceneryPaths = modifiedPaths;
                            }
                        }
                    }

                    StyledText {
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
                        if (path !== "") {
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

                StyledText {
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

    Scrollbar {
        id: scrollbar
        anchors.right: parent.right
        height: parent.height
        flickable: flick
        visible: flick.contentHeight > flick.height
    }
}

