import QtQuick 2.4
import "."

Text {
    signal dismiss();

    readonly property string updateAllLink: "\"launcher:update-all\"";
    readonly property string newName: _notifications.argsForIndex(model.index).newCatalogName

    text: qsTr("An updated version of the official aircraft hangar '%2' was automatically installed. " +
               "Existing aircraft have been marked for update, <a href=%1>click here to update them all</a>").arg(updateAllLink).arg(newName)

    wrapMode: Text.WordWrap
    font.pixelSize: Style.subHeadingFontPixelSize
    color: "white"

    onLinkActivated: {
        _launcher.requestUpdateAllAircraft();
        dismiss(); // request our dismissal
    }
}
