import QtQuick 2.4
import "."

Text {
    property bool enabled: true

    color: enabled ? Style.baseTextColor : Style.disabledTextColor

    // make the text slightly smaller?

    wrapMode: Text.WordWrap
}
