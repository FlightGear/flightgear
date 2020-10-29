import QtQuick 2.4
import FlightGear.Launcher 1.0
import "."

Rectangle {
    id: root
    color: "white"
    property alias aircraftURI: aircraft.uri

    MouseArea {
        // consume all mouse-clicks on the detail view
        anchors.fill: parent
    }

    AircraftInfo
    {
        id: aircraft
    }

    ScrolledFlickable
    {
        id: flickable

        anchors.fill: parent
        contentWidth: parent.width
        contentHeight: content.childrenRect.height
        boundsBehavior: Flickable.StopAtBounds

        Item {
            id: content
            width: root.width - scrollbar.width
            height: childrenRect.height

            Column {
                width: content.width - (Style.margin * 2)
                spacing: Style.margin
                anchors.horizontalCenter: parent.horizontalCenter

                Item { // top padding
                    width: parent.width
                    height: Style.margin
                }

                AircraftVariantChoice {
                    id: headingBox
                    fontPixelSize: Style.headingFontPixelSize * 2
                    popupFontPixelSize: Style.headingFontPixelSize

                    anchors {
                        margins: Style.strutSize * 2 // space for back button
                        left: parent.left
                        right: parent.right
                    }

                    aircraft: aircraftURI
                    currentIndex: aircraft.variant

                    onSelected: {
                        aircraft.variant = index
                        _launcher.selectedAircraft = aircraft.uri;
                    }
                }

                // this element normally hides itself unless needed
                AircraftWarningPanel {
                    id: warningBox
                    aircraftStatus: aircraft.status
                    requiredFGVersion: aircraft.minimumFGVersion
                    width: parent.width
                }

                // thumbnails + description + authors container
                Item {
                    width: parent.width
                    height: childrenRect.height

                    Rectangle {
                        id: thumbnailBox
                        // thumbnail border

                        border.width: 1
                        border.color: "#7f7f7f"

                        width: thumbnail.width
                        height: thumbnail.height

                        ThumbnailImage {
                            id: thumbnail

                            aircraftUri: root.aircraftURI
                            maximumSize.width: 172
                            maximumSize.height: 128
                        }
                    }

                    Column {
                        anchors.left: thumbnailBox.right
                        anchors.leftMargin: Style.margin
                        anchors.right: parent.right
                        spacing: Style.margin

                        StyledText {
                            id: aircraftDescription
                            text: aircraft.description
                            width: parent.width
                            wrapMode: Text.WordWrap
                            visible: aircraft.description != ""
                        }

                        StyledText {
                            id: aircraftAuthors
                            text: qsTr("by %1").arg(aircraft.authors)
                            width: parent.width
                            anchors.horizontalCenter: parent.horizontalCenter
                            wrapMode: Text.WordWrap
                            visible: (aircraft.authors != undefined)
                        }
                    }

                }

                // web-links row
                Row {
                    width: parent.width
                    height: childrenRect.height
                    spacing: Style.margin

                    Weblink {
                        visible: aircraft.homePage != ""
                        label: qsTr("Website")
                        link: aircraft.homePage
                    }

                    Weblink {
                        visible: aircraft.supportUrl != ""
                        label: qsTr("Support and issue reporting")
                        link: aircraft.supportUrl
                    }

                    Weblink {
                        visible: aircraft.wikipediaUrl != ""
                        label: qsTr("Wikipedia")
                        link: aircraft.wikipediaUrl
                    }
                }

                AircraftDownloadPanel {
                    visible: aircraft.isPackaged
                    width: parent.width
                    uri: aircraft.uri
                    installStatus: aircraft.installStatus
                    packageSize: aircraft.packageSize
                    downloadedBytes: aircraft.downloadedBytes
                }

                AircraftPreviewPanel {
                    id: previews
                    width: parent.width
                    previews: aircraft.previews
                    visible: aircraft.previews.length > 0
                }

                Row {
                    height: ratingGrid.height
                    width: parent.width
                    spacing: Style.strutSize

                    FavouriteToggleButton {
                        checked: aircraft.favourite
                        onToggle: {
                            aircraft.favourite = on;
                        }
                    }

                    Grid {
                        id: ratingGrid

                        visible: aircraft.ratings !== undefined

                        rows: 2
                        columns: 3
                        rowSpacing: Style.margin
                        columnSpacing: Style.margin

                        StyledText {
                            id: ratingsLabel
                            text: qsTr("Ratings:")
                        }


                        AircraftRating {
                            title: qsTr("Flight model")
                            Binding on value {
                                when: aircraft.ratings !== undefined
                                value: aircraft.ratings[0]
                            }
                        }

                        AircraftRating {
                            title: qsTr("Systems")
                            Binding on value {
                                when: aircraft.ratings !== undefined
                                value: aircraft.ratings[1]
                            }
                        }

                        Item {
                            width: ratingsLabel.width
                            height: 1
                        } // placeholder

                        AircraftRating {
                            title: qsTr("Cockpit")
                            Binding on value {
                                when: aircraft.ratings !== undefined
                                value: aircraft.ratings[2]
                            }
                        }

                        AircraftRating {
                            title: qsTr("Exterior")
                            Binding on value {
                                when: aircraft.ratings !== undefined
                                value: aircraft.ratings[3]
                            }
                        }
                    } // of rating grid
                }

                StyledText {
                    text: qsTr("Local file location: %1").arg(aircraft.pathOnDisk);
                    width: parent.width
                    wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                    visible: aircraft.pathOnDisk != undefined
                }

            } // main layout column
        } // of main item

    } // of Flickable

    FGCompatScrollbar {
        id: scrollbar
        anchors.right: parent.right
        anchors.top: parent.top
        height: parent.height
        flickable: flickable
        visible: flickable.visibleArea.heightRatio < 1.0
    }
} // of Rect
