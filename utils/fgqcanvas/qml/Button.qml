import QtQuick 2.0

Rectangle {
    id: root

    property alias label: labelText.text
    property bool enabled: true

    signal clicked

    border.width: 2
    border.color: enabled ? "orange" : "9f9f9f"

    color: "#3f3f3f"
    implicitWidth: 100
    implicitHeight: 30

    Text {
        id: labelText
        anchors.centerIn: parent
        color: enabled ? "white" : "9f9f9f"
    }

    MouseArea {
        anchors.fill: parent
        enabled: root.enabled

        onClicked: {
            root.clicked();
        }
    }
}
