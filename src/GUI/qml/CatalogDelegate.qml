import QtQuick 2.4
import FlightGear.Launcher 1.0
import "."

Item {
    id: delegateRoot

    // don't show the delegate for newly added catalogs, until the
    // adding process is complete
    visible: !model.isNewlyAdded

    implicitWidth: 300
    height: childrenRect.height

    Item {
        id: divider
        visible: model.index > 0 // divider before each item except the first
        height: Style.margin
        width: parent.width

        Rectangle {
            color: Style.frameColor
            height: 1
            width: parent.width - Style.strutSize
            anchors.centerIn: parent
        }
    }

    function isDisabled()
    {
        return (model.status !== CatalogListModel.Ok) ||
                (enableCheckbox.checked === false);
    }

    Item
    {
        anchors.top: divider.bottom
        height: catalogTextColumn.childrenRect.height + Style.margin * 2
        width: parent.width


        Column {
            id: catalogTextColumn

            y: Style.margin
            anchors.left: parent.left
            anchors.leftMargin: Style.margin
            anchors.right: catalogDeleteButton.left
            anchors.rightMargin: Style.margin
            spacing: Style.margin

            Row {
                spacing: Style.margin
                height: headingText.height

                Checkbox {
                    id: enableCheckbox
                    checked: model.enabled
                    height: parent.height
                    onCheckedChanged: {
                        model.enabled = checked;
                    }
                    // only allow the user to toggle enable/disable if
                    // the catalog is valid
                    visible: (model.status === CatalogListModel.Ok)
                }

                StyledText {
                    id: headingText
                    font.pixelSize: Style.subHeadingFontPixelSize
                    font.bold: true
                    width: catalogTextColumn.width - enableCheckbox.width
                    text: model.name
                    font.strikeout: delegateRoot.isDisabled();
                }
            }

            StyledText {
                id: normalDescription
                visible: model.status === CatalogListModel.Ok
                width: parent.width
                text: model.description
                wrapMode: Text.WordWrap
            }

            StyledText {
                id: badVersionDescription
                visible: model.status === CatalogListModel.IncompatibleVersion
                width: parent.width
                wrapMode: Text.WordWrap
                text: qsTr("This hangar is not compatible with this version of FlightGear")
            }

            ClickableText {
                id: errorDescription
                visible: (model.status !== CatalogListModel.Ok) && (model.status !== CatalogListModel.IncompatibleVersion)
                width: parent.width
                wrapMode: Text.WordWrap
                text: qsTr("This hangar is currently disabled due to a problem. " +
                           "Click here to try updating the hangar information from the server. "
                           + "(An Internet connection is required for this)");
                onClicked:  {
                    _addOns.catalogs.refreshCatalog(model.index)
                }
            }

            StyledText {
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
            anchors.margins: 1 // don't cover the border

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

            Rectangle {
                anchors.fill: parent
                z: -1
            }
        }
    }

}
