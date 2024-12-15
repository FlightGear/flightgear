import QtQuick 2.4
import FlightGear.Launcher 1.0
import FlightGear 1.0

Item {
    id: root

    signal select(var uri);
    signal showDetails(var uri)

    implicitHeight: Math.max(contentBox.childrenRect.height, thumbnailBox.height) +
                    footer.height + Style.margin / 2
    implicitWidth: ListView.view.width

    readonly property bool __isSelected: (_launcher.selectedAircraft === model.uri)

    property bool __showAlternateText: false

    function cycleTextDisplay()
    {
        __showAlternateText = !__showAlternateText;
    }

    function alternateText()
    {
        return qsTr("URI: %1\nLocal path: %2").arg(model.uri).arg(titleBox.pathOnDisk);
    }

    MouseArea {
        anchors.fill: parent

        onClicked: function(mouse) {
            if (mouse.modifiers & Qt.ShiftModifier ) {
                root.cycleTextDisplay();
                return;
            }

            if (__isSelected) {
                root.showDetails(model.uri)
            } else {
                root.select(model.uri)
            }
        }
    }

    Item {
        id: thumbnailBox
        width: 172
        height: 128
        anchors.top: parent.top
        anchors.topMargin: Style.margin / 2

        Rectangle {
            anchors.centerIn: parent
            border.width: 1
            border.color: "#7f7f7f"
            width: thumbnail.width
            height: thumbnail.height

            ThumbnailImage {
                id: thumbnail
                aircraftUri: model.uri
                maximumSize.width: 172
                maximumSize.height: 128
            }
        }
    }

    Column {
        id: contentBox

        anchors {
            left: thumbnailBox.right
            right: parent.right
            leftMargin: Style.margin
            rightMargin: Style.margin
            topMargin: Style.margin / 2
            top: parent.top
        }

        spacing: Style.margin

        Item {
            height: titleBox.height
            width: parent.width

            FavouriteToggleButton {
                id: favourite
                checked: model.favourite
                anchors.verticalCenter: parent.verticalCenter
                onToggle: {
                    model.favourite = on;
                }
            }

            AircraftVariantChoice {
                id: titleBox

                anchors {
                    left: favourite.right
                    leftMargin: Style.margin
                    right: parent.right
                }

                aircraft: model.uri;
                currentIndex: model.activeVariant
                onSelected: {
                    model.activeVariant = index
                    root.select(model.uri)
                }

                GettingStartedTip {
                    tipId: "aircraftVariantTip"
                    enabled: model.variantCount > 0
                    standalone: true

                    Component.onCompleted: {
                        if (enabled) {
                           showOneShot();
                        }
                    }

                    anchors {
                        horizontalCenter: parent.horizontalCenter
                        top: parent.bottom
                    }
                    arrow: GettingStartedTip.TopCenter
                    text: qsTr("Click here to select different variants or models of this aircraft")
                }
            }
        }



        StyledText {
            id: description
            width: parent.width
            text: root.__showAlternateText ? root.alternateText()
                                           : model.description
            maximumLineCount: 3
            wrapMode: Text.WordWrap
            elide: Text.ElideRight
            height: implicitHeight
            visible: (model.description !== "") || root.__showAlternateText
        }

        AircraftDownloadPanel
        {
            id: downloadPanel
            visible: (model.package !== undefined)
            packageSize: model.packageSizeBytes
            installStatus: model.packageStatus
            downloadedBytes: model.downloadedBytes
            uri: model.uri
            width: parent.width
            compact: true
        }

    } // of content column

    Item {
        id: footer
        height: Style.margin / 2 + 1
        width: parent.width
        anchors.bottom: parent.bottom

        Rectangle {
            color: Style.frameColor
            height: 1
            width: parent.width - Style.strutSize
            anchors.bottom: parent.bottom
            anchors.horizontalCenter: parent.horizontalCenter
        }
    }
} // of root item
