import QtQuick 2.4
import FlightGear 1.0

Item {
    id: root
    property bool checked: false
    property alias label: label.text
    property bool enabled: true

    implicitWidth: track.width + label.width + 16
    implicitHeight: Math.max(label.height, thumb.height)

    // helepr to set the value without an animation
    function setValue(newCheck)
    {
        sliderBehaviour.enabled = false;
        checked = newCheck
        sliderBehaviour.enabled = true;
    }

    function toggle(newChecked)
    {
        checked = newChecked
    }

    Rectangle {
        id: track
        width: height * 2
        height: radius * 2
        radius: Style.roundRadius

        color: (checked && root.enabled) ? Style.frameColor : Style.minorFrameColor
        anchors.left: parent.left
        anchors.leftMargin: Style.margin
        anchors.verticalCenter: parent.verticalCenter

        Rectangle {
            id: thumb
            width: radius * 2
            height: radius * 2
            radius: Style.roundRadius * 1.5

            anchors.verticalCenter: parent.verticalCenter
            color: root.enabled && (checked | mouseArea.containsMouse) ? Style.themeColor : "white"
            border.width: 1
            border.color: Style.inactiveThemeColor

            x: checked ? parent.width - (track.radius + radius) : (track.radius - radius)

            Behavior on x {
                id: sliderBehaviour
                NumberAnimation {
                    duration: 250
                }
            }
        }
    }

    StyledText {
        id: label
        anchors.left: track.right
        anchors.leftMargin: Style.margin
        anchors.verticalCenter: parent.verticalCenter
        enabled: root.enabled
        hover: mouseArea.containsMouse
    }

    MouseArea {
        anchors.fill: parent
        id: mouseArea
        enabled: root.enabled

        hoverEnabled: true
        onClicked: root.toggle(!checked)
        cursorShape: enabled ? Qt.PointingHandCursor : Qt.ArrowCursor

    }
}
