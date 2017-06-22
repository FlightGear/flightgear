import QtQuick 2.0
import FlightGear.Launcher 1.0

Item
{
    id: root

    readonly property int margin: 8

    Component
    {
        id: aircraftDelegate

        Item {
            // background

            height: Math.max(contentBox.childrenRect.height, thumbnailBox.height) + footer.height
            width: root.width

            MouseArea {
                anchors.fill: parent
                onClicked: {
                    list.currentIndex = model.index
                    _launcher.selectAircraft(model.uri);
                }
            }

            Rectangle {
                id: thumbnailBox
                // thumbnail border

                y: Math.max(0, Math.round((contentBox.childrenRect.height - height) * 0.5))

                border.width: 1
                border.color: "#7f7f7f"

                width: thumbnail.width
                height: thumbnail.height

                ThumbnailImage {
                    id: thumbnail

                    aircraftUri: model.uri
                    maximumSize.width: 300
                    maximumSize.height: 200
                }

                Image {
                    id: previewIcon
                    visible: model.hasPreviews
                    anchors.left: parent.left
                    anchors.bottom: parent.bottom
                    source: "qrc:///preview-icon"
                    opacity: showPreviewsMouse.containsMouse ? 1.0 : 0.5
                    Behavior on opacity {
                        NumberAnimation { duration: 100 }
                    }
                }

                MouseArea {
                    id: showPreviewsMouse
                    hoverEnabled: true
                    anchors.fill: parent
                    visible: model.hasPreviews

                    onClicked: {
                        _launcher.showPreviewsFor(model.uri);
                    }
                }
            }

           Column {
               id: contentBox

               anchors.leftMargin: root.margin
               anchors.left: thumbnailBox.right
               anchors.right: parent.right
               anchors.rightMargin: root.margin

               spacing: root.margin

               Item
               {
                   // box to contain the aircraft title and the variant
                   // selection button arrows

                   id: headingBox
                   width: parent.width
                   height: title.height

                   ArrowButton {
                       id: previousVariantButton

                       visible: (model.variantCount > 0) && (model.activeVariant > 0)
                       arrow: "qrc:///left-arrow-icon"
                       anchors.verticalCenter: parent.verticalCenter
                       anchors.left: parent.left

                       onClicked: {
                           model.activeVariant = (model.activeVariant - 1)
                       }
                   }

                   Text {
                       id: title

                       anchors.verticalCenter: parent.verticalCenter
                       anchors.left: previousVariantButton.right
                       anchors.right: nextVariantButton.left

                       horizontalAlignment: Text.AlignHCenter
                       font.pixelSize: 24
                       text: model.title

                       elide: Text.ElideRight
                   }

                   ArrowButton {
                       id: nextVariantButton

                       arrow: "qrc:///right-arrow-icon"
                       visible: (model.variantCount > 0) && (model.activeVariant < model.variantCount - 1)
                       anchors.verticalCenter: parent.verticalCenter
                       anchors.right: parent.right

                       onClicked: {
                           model.activeVariant = (model.activeVariant + 1)
                       }
                   }

               } // top header box (title + variant arrows)

               // this element normally hides itself unless needed
               AircraftWarningPanel {
                   id: warningBox
                   aircraftStatus: model.aircraftStatus
                   requiredFGVersion: model.requiredFGVersion
                   width: parent.width
               }

               Text {
                   id: authors
                   visible: (model.authors != undefined)
                   width: parent.width
                   text: "by: " + model.authors
                   maximumLineCount: 3
                   wrapMode: Text.WordWrap
                   elide: Text.ElideRight
               }

               Text {
                   id: description
                   width: parent.width
                   text: model.description
                   maximumLineCount: 10
                   wrapMode: Text.WordWrap
                   elide: Text.ElideRight
               }

               Item {
                   // this area holds the install/update/remove button,
                   // and the ratings grid. when a download / update is
                   // happening, the content is re-arranged to give more room
                   // for the progress bar and feedback
                   id: bottomContent

                   readonly property int minimumWidthForBottomContent: ratingGrid.width + downloadPanel.compactWidth + root.margin

                   width: Math.max(parent.width, minimumWidthForBottomContent)
                   height: ratingGrid.height

                   AircraftDownloadPanel
                   {
                       id: downloadPanel
                       visible: (model.package != undefined)
                       packageSize: model.packageSizeBytes
                       installStatus: model.packageStatus
                       downloadedBytes: model.downloadedBytes
                       uri: model.uri
                       width: parent.width // full width, grid sits on top
                   }

                   Grid {
                       id: ratingGrid
                       anchors.right: parent.right

                       // hide ratings when the panel is doing something, to
                       // make more room for the progress bar and text
                       visible: model.hasRatings && !downloadPanel.active

                       rows: 2
                       columns: 2
                       rowSpacing: root.margin
                       columnSpacing: root.margin

                       AircraftRating {
                           title: "Flight model"
                           value: model.ratingFDM;
                       }

                       AircraftRating {
                           title: "Systems"
                           value: model.ratingSystems;
                       }

                       AircraftRating {
                           title: "Cockpit"
                           value: model.ratingCockpit;
                       }

                       AircraftRating {
                           title: "Exterior"
                           value: model.ratingExterior;
                       }
                   }
               }
           } // of content column

           Item {
               id: footer
               height: 12
               width: parent.width
               anchors.bottom: parent.bottom

               Rectangle {
                   color: "#68A6E1"
                   height: 2
                   width: parent.width - 60
                   anchors.centerIn: parent
               }
           }
        } // of Component root item
    } // of Component

    Component {
        id: highlight
        Rectangle {
            gradient: Gradient {
                      GradientStop { position: 0.0; color: "#98A3B4" }
                      GradientStop { position: 1.0; color: "#5A6B83" }
                  }
        }
    }

    ListView {
        id: list
        model: _filteredModel
        anchors.fill: parent
        delegate: aircraftDelegate

        highlight: highlight
        highlightMoveDuration: 100

        Keys.onUpPressed: {

        }

        Keys.onDownPressed: {

        }

        Scrollbar {
            anchors.right: parent.right
            height: parent.height
        }
    }

    Connections
    {
        target: _launcher
        onSelectAircraftIndex: {
            console.warn("Selecting aircraft:" + index);
            list.currentIndex = index;
        }
    }

    Rectangle {
        id: updateAllBox

        visible: _aircraftModel.showUpdateAll
        width: parent.width
        height: updateAllRow.childrenRect.height + root.margin * 2

        Row {
            y: root.margin
            id: updateAllRow
            spacing: root.margin

            Text {
                text: _aircraftModel.aircraftNeedingUpdated + " aircraft have updates available - download and install them now?"
                wrapMode: Text.WordWrap
                anchors.verticalCenter: parent.verticalCenter
            }

            Button {
                text: "Update all"
                anchors.verticalCenter: parent.verticalCenter

                onClicked: {
                    _launcher.requestUpdateAllAircraft();
                    _launcher.showUpdateAll = false
                }
            }

            Button {
                text: "Not now"
                anchors.verticalCenter: parent.verticalCenter

                onClicked: {
                    _launcher.showUpdateAll = false
                }
            }
        }
    } // of update-all prompt

    Rectangle {
        id: noDefaultCatalogBox
        visible: _launcher.showNoOfficialHanger
        width: parent.width
        height: noDefaultCatalogRow.childrenRect.height + root.margin * 2

        Row {
            y: root.margin
            id: noDefaultCatalogRow
            spacing: root.margin

            Text {
                text: "The official FlightGear aircraft hangar is not added, so many standard "
                      + "aircraft will not be available. You can add the  hangar now, or hide "
                      + "this message. The offical hangar can always be restored from the 'Add-Ons' page."
                wrapMode: Text.WordWrap
                anchors.verticalCenter: parent.verticalCenter
                width: noDefaultCatalogBox.width - (addDefaultButton.width + hideButton.width + root.margin * 3)
            }

            Button {
                id: addDefaultButton
                text: qsTr("Add default hangar")
                anchors.verticalCenter: parent.verticalCenter

                onClicked: {
                    _launcher.officialCatalogAction("add-official");
                }
            }

            Button {
                id: hideButton
                text: qsTr("Hide")
                anchors.verticalCenter: parent.verticalCenter

                onClicked: {
                    _launcher.officialCatalogAction("hide");
                }
            }
        }
    }

    // no default catalog pop-over
}

