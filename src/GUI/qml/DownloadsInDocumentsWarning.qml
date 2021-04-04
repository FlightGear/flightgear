import QtQuick 2.4
import FlightGear 1.0

Text {
    signal dismiss();

    readonly property string oldLocationURI: "\"" + _notifications.argsForIndex(model.index).oldLocation + "\""
    readonly property string newLocationURI: "\"" + _notifications.argsForIndex(model.index).newLocation + "\""

    text: qsTr("<p>FlightGear previously downloaded aircraft and scenery to a folder within your 'Documents' folder. " +
               "This can cause problems with some security features of Windows, so a new location is now recommended.</p><br/>" +
               "<p>To keep your existing aircraft and scenery downloads, please move the files from " +
               "<u><a href=%1>the old location</a></u> to <u><a href=%2>the new location</a></u></p>").arg(oldLocationURI).arg(newLocationURI)

    wrapMode: Text.WordWrap
    font.pixelSize: Style.subHeadingFontPixelSize
    color: Style.themeContrastTextColor
    linkColor: Style.themeContrastLinkColor

    textFormat: Text.StyledText

    onLinkActivated: {
        console.log("Activated:" + link);
        Qt.openUrlExternally(link); // will open Windows Explorer since it's a file:/// URI
        // don't dismiss, user needs to click both links :)
    }
}
