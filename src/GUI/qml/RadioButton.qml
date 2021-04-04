import QtQuick 2.4
import FlightGear 1.0

Item {
    id: root
    property bool selected
    property RadioButtonGroup group // nil by default

    signal clicked()

    implicitHeight: outerRing.height + Style.margin
    implicitWidth: outerRing.width + Style.margin

    function select()
    {
        if (root.group) {
            root.group.selected = root;
        }
    }

    Binding {
        when: root.group != null
        target: root
        property: "selected"
        value: (root.group.selected === root)
    }

    Rectangle {
        id: innerRing
        anchors.centerIn: parent
        width: radius * 2
        height: radius * 2
        radius: Style.roundRadius
        color: Style.themeColor
        visible: selected || mouse.containsMouse

    }

    Rectangle {
        id: outerRing
        anchors.centerIn: parent
        border.color: Style.themeColor
        border.width: 2
        color: "transparent"
        radius: Style.roundRadius + 4
        width: radius * 2
        height: radius * 2
    }

    Rectangle {
        id: pressRing
        opacity: 0.3
        width: outerRing.width * 2
        height: outerRing.height * 2
        visible: mouse.pressed
        radius: width / 2
        color: "#7f7f7f"
        anchors.centerIn: parent
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        onClicked: {
            root.select();
            root.clicked()
        }
    }
}
