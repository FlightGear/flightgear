import QtQuick 2.4
import QtQml 2.4

Item {
    id: root
    property alias timeout: showTimeout.interval

    property bool atTheBottom: false
    property bool didTimeout: false

    opacity: didTimeout && !atTheBottom

    width: image.width
    height: image.height

    Timer {
        id: showTimeout
        running: true
        onTriggered: didTimeout = true;
    }

    Behavior on opacity {
        NumberAnimation {
            duration: 800
            easing: Easing.InOutQuad
        }
    }

    Image {
        id: image
        source: "qrc:///scroll-down-icon-white"
    }
}
