
import QtQuick 2.4
import FlightGear.Launcher 1.0
import "."

ClickableText {
    signal dismiss();

    text: msg.arg(_updates.updateVersion)
    readonly property string msg: (_updates.status == UpdateChecker.MajorUpdate) ?
                                      qsTr("A new release of FlightGear is available (%1): click for more information")
                                    : qsTr("Updated version %1 is available: click here to download")


    wrapMode: Text.WordWrap
    font.pixelSize: Style.subHeadingFontPixelSize
    color: "white"

    onClicked: {
        _launcher.launchUrl(_updates.updateUri);
    }

    function dismissed()
    {
        _updates.ignoreUpdate();
    }
}

