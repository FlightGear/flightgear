import QtQuick 2.4
import FlightGear.Launcher 1.0
import "."

Item {
    id: root

    signal select(var uri);
    signal showDetails(var uri)

    implicitHeight: Math.max(contentBox.childrenRect.height, thumbnailBox.height) +
                    footer.height
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

        onClicked: {
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
            maximumSize.width: 172
            maximumSize.height: 128
        }
    }

    Column {
        id: contentBox

        anchors {
            left: thumbnailBox.right
            right: parent.right
            leftMargin: Style.margin
            rightMargin: Style.margin
            top: parent.top
        }

        spacing: Style.margin

        AircraftVariantChoice {
            id: titleBox

            width: parent.width

            aircraft: model.uri;
            currentIndex: model.activeVariant
            onSelected: {
                model.activeVariant = index
                root.select(model.uri)
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
} // of root item
