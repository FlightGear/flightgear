import QtQuick 2.4
import FlightGear 1.0

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
            return b ? "image://colored-icon/star-filled?theme" : "image://colored-icon/star-outline?text";
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
