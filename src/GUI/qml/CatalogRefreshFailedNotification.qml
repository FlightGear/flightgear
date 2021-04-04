import QtQuick 2.4
import FlightGear 1.0

Text {
    signal dismiss();

    readonly property string failUri: _notifications.argsForIndex(model.index).catalogUri

    text: qsTr("The catalog at '%1' failed to download and validate correctly. All aircraft it provides will be unavailable.").arg(failUri)

    wrapMode: Text.WordWrap
    font.pixelSize: Style.subHeadingFontPixelSize
    color: "white"
}
