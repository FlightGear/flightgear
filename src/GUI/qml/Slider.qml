import QtQuick 2.4
import FlightGear 1.0

Item {
    id: root
    property alias label: labelText.text

    property int min: 0
    property int max: 100
    property int value: 50

    property int sliderWidth: 0

    readonly property real __percentFull: value / (max - min)

    implicitHeight: labelText.height
    implicitWidth: labelText.width * 2

    Text {
        id: labelText
        width: parent.width - (emptyTrack.width + Style.margin)
        horizontalAlignment: Text.AlignRight
        font.pixelSize: Style.baseFontPixelSize
    }

    Rectangle {
        id: emptyTrack

        width: sliderWidth > 0 ? sliderWidth : parent.width - (labelText.implicitWidth + Style.margin)

        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter

        height: 2
        color: Style.inactiveThemeColor

        Rectangle {
            id: fullTrack
            height: parent.height
            color: Style.frameColor
            width: parent.width * __percentFull
        }

        MouseArea {
            id: clickTrackArea
            anchors.centerIn: parent
            width: parent.width
            height: root.height

            onClicked: {
                var frac = mouse.x / width;
                root.value = min + 0.5 + (max - min) * frac;
            }
        }

        // invisble item that moves directly with the mouse
        Item {
            id: dragThumb
            height: 2
            width: 2

            anchors.verticalCenter: parent.verticalCenter

            Drag.active: thumbMouse.drag.active

            onXChanged:  {
                if (thumbMouse.drag.active) {
                    var frac = x / emptyTrack.width;
                    value = min + (max - min) * frac;
                }
            }
        }

        Rectangle {
            id: thumb
            width: radius * 2
            height: radius * 2
            radius: Style.roundRadius
            color: Style.themeColor

            anchors.verticalCenter: parent.verticalCenter
            x: parent.width * __percentFull



            MouseArea {
                id: thumbMouse
                hoverEnabled: true
                anchors.fill: parent

                drag.axis: Drag.XAxis
                drag.minimumX: 0
                drag.maximumX: emptyTrack.width
                drag.target: dragThumb

                onPressed: {
                     dragThumb.x = emptyTrack.width * __percentFull
                }
            }
        }


    } // of base track rectangle

}
