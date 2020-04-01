import QtQuick 2.4
import "."

Item {
    id: root
    property bool checked: false

    implicitWidth: icon.width + Style.margin
    implicitHeight: icon.height + Style.margin

    signal toggle(var on);


    Image {
        id: icon
        source: {
            var b = mouse.containsMouse ? !root.checked : root.checked;
            return b ? "qrc:///favourite-icon-filled" : "qrc:///favourite-icon-outline";
        }

        anchors.centerIn: parent
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        onClicked: root.toggle(!root.checked);

        cursorShape: Qt.PointingHandCursor
    }
}
