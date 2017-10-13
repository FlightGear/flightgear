import QtQuick 2.0

Item {
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
        width: parent.width - (emptyTrack.width + 8)
        horizontalAlignment: Text.AlignRight
    }

    Rectangle {
        id: emptyTrack

        width: sliderWidth > 0 ? sliderWidth : parent.width - (labelText.implicitWidth + 8)

        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter

        height: 2
        color: "#9f9f9f"

        Rectangle {
            id: fullTrack
            height: parent.height
            color:"#68A6E1"
            width: parent.width * __percentFull
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
            width: 12
            height: 12
            radius: 6
            color: "#68A6E1"

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
