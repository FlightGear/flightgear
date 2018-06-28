import QtQuick 2.2
import FlightGear.Launcher 1.0 as FG

Item {
    id: root

    height: frameRect.height + (Style.margin * 2);
    z: 100

    Rectangle {
        id: frameRect
        y: Style.margin
        height: noDefaultCatalogRow.childrenRect.height + (Style.margin * 2);
        width: parent.width
        color: "white"
        border.width: 1
        border.color: Style.minorFrameColor

        Row {
            y: Style.margin
            x: Style.margin

            id: noDefaultCatalogRow
            spacing: Style.margin

            StyledText {
                text: qsTr("The official FlightGear aircraft hangar is not added, so many standard "
                      + "aircraft will not be available. You can add the  hangar now, or hide "
                      + "this message. The offical hangar can always be restored from the 'Add-Ons' page.")
                wrapMode: Text.WordWrap
                anchors.verticalCenter: parent.verticalCenter
                width: root.width - (addDefaultButton.width + hideButton.width + Style.margin * 3)
            }

            Button {
                id: addDefaultButton
                text: _addOns.catalogs.isAddingCatalog ? qsTr("Adding hangar")
                                                       : qsTr("Add default hangar")
                anchors.verticalCenter: parent.verticalCenter
                onClicked: _addOns.officialCatalogAction("add-official");
                enabled: !_addOns.catalogs.isAddingCatalog
            }

            Button {
                id: hideButton
                text: qsTr("Hide")
                anchors.verticalCenter: parent.verticalCenter
                onClicked: _addOns.officialCatalogAction("hide");
            }
        } // content row
    } // visible frame
} // root item
